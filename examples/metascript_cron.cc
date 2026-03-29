#include "../metascript/runtime.hh"
#include <iostream>

const char* html = R"html(
<!DOCTYPE html>
<html>
<head>
    <title>MetaScript Cron Demo</title>
    <style>
        body { font-family: sans-serif; padding: 20px; }
        pre { background: #f0f0f0; padding: 10px; }
        .success { color: green; }
        .error { color: red; }
    </style>
</head>
<body>
    <h1>MetaScript Cron Demo</h1>

    <div>
        <h3>Parse Cron Expression</h3>
        <input type="text" id="expr" value="*/15 * * * *">
        <button onclick="parseCron()">Parse</button>
        <p id="parseResult"></p>
    </div>

    <hr>

    <div>
        <h3>Register Cron Job</h3>
        <p>This will register a job that runs <code>worker.ts</code> every Monday at 2:30 AM.</p>
        <button onclick="registerJob()">Register Job</button>
        <button onclick="removeJob()">Remove Job</button>
        <p id="registerResult"></p>
    </div>

    <script>
        async function parseCron() {
            const expr = document.getElementById('expr').value;
            try {
                const next = await Meta.cron.parse(expr);
                document.getElementById('parseResult').textContent = 'Next: ' + (next ? next.toISOString() : 'null');
                document.getElementById('parseResult').className = 'success';
            } catch (e) {
                document.getElementById('parseResult').textContent = 'Error: ' + e.message;
                document.getElementById('parseResult').className = 'error';
            }
        }

        async function registerJob() {
            try {
                await Meta.cron("./worker.ts", "30 2 * * MON", "weekly-report");
                document.getElementById('registerResult').textContent = 'Job "weekly-report" registered!';
                document.getElementById('registerResult').className = 'success';
            } catch (e) {
                document.getElementById('registerResult').textContent = 'Error: ' + e.message;
                document.getElementById('registerResult').className = 'error';
            }
        }

        async function removeJob() {
            try {
                await Meta.cron.remove("weekly-report");
                document.getElementById('registerResult').textContent = 'Job "weekly-report" removed!';
                document.getElementById('registerResult').className = 'success';
            } catch (e) {
                document.getElementById('registerResult').textContent = 'Error: ' + e.message;
                document.getElementById('registerResult').className = 'error';
            }
        }
    </script>
</body>
</html>
)html";

int main(int argc, char** argv) {
    // Handle CLI for scheduled execution
    int res = metascript::runtime::handle_cli(argc, argv);
    if (res != -1) {
        return res;
    }

    try {
        metascript::runtime rt(true);
        rt.set_title("MetaScript Cron Demo");
        rt.set_size(600, 500, WEBVIEW_HINT_NONE);
        rt.set_html(html);
        rt.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
