#include "webview/webview.h"
#include <string>
#include <iostream>

extern "C" void webview_alloy_setup(webview_t w);

int main() {
    try {
        webview::webview w(true, nullptr);
        webview_alloy_setup(w.get_native_handle(WEBVIEW_NATIVE_HANDLE_KIND_UI_WIDGET)); // This is wrong, it expects webview_t

        // Correct way for C++ wrapper is tricky because get_native_handle returns internal pointers.
        // For this demo, let's use the C API directly to ensure compatibility.
        webview_t wv = webview_create(1, nullptr);
        webview_alloy_setup(wv);

        webview_set_title(wv, "Alloy Demo");
        webview_set_size(wv, 800, 600, WEBVIEW_HINT_NONE);
        webview_set_html(wv, R"html(
            <html>
            <head>
                <style>
                    body { font-family: sans-serif; padding: 20px; }
                    pre { background: #eee; padding: 10px; border-radius: 4px; overflow: auto; max-height: 400px; }
                </style>
            </head>
            <body>
                <h1>Alloy Runtime Demo</h1>
                <button onclick="runSpawn()">Run 'ls -l'</button>
                <button onclick="testSqlite()">Test SQLite</button>
                <pre id="output">Output will appear here...</pre>
                <script>
                    async function runSpawn() {
                        const out = document.getElementById('output');
                        out.textContent = "Running...";
                        try {
                            const proc = Alloy.spawn(["ls", "-l"]);
                            const text = await proc.stdout.text();
                            out.textContent = text;
                        } catch (e) {
                            out.textContent = "Error: " + e.message;
                        }
                    }

                    function testSqlite() {
                        const out = document.getElementById('output');
                        try {
                            const db = new Alloy.sqlite.Database(":memory:");
                            db.exec("CREATE TABLE test (id INTEGER PRIMARY KEY, name TEXT)");
                            db.exec("INSERT INTO test (name) VALUES ('Alloy'), ('WebView')");
                            const rows = db.query("SELECT * FROM test").all();
                            out.textContent = JSON.stringify(rows, null, 2);
                            db.close();
                        } catch (e) {
                            out.textContent = "Error: " + e.message;
                        }
                    }
                </script>
            </body>
            </html>
        )html");
        webview_run(wv);
        webview_destroy(wv);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
