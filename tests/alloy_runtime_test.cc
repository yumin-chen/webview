#include "webview/webview.h"
#include <iostream>
#include <cassert>

int main() {
    try {
        webview::webview w(true, nullptr);

        // Test SQLite in host
        w.dispatch([&]() {
            w.eval(R"js(
                Alloy.secureEval(`
                    const db = new Alloy.sqlite.Database(":memory:");
                    db.exec("CREATE TABLE t(x)");
                    db.exec("INSERT INTO t VALUES (1), (2), (3)");
                    const res = db.exec("SELECT SUM(x) as s FROM t");
                    if (res[0].s !== 6) throw new Error("SQLite failed: " + JSON.stringify(res));
                    console.log("SQLite test passed!");
                `);
            )js");
        });

        // Test Streams in host
        w.dispatch([&]() {
            w.eval(R"js(
                Alloy.secureEval(`
                    const sink = new Alloy.ArrayBufferSink();
                    sink.start();
                    sink.write("hello");
                    const res = sink.end();
                    if (res !== "hello") throw new Error("Sink failed: " + res);

                    const stream = new ReadableStream({
                        start(controller) {
                            controller.enqueue("a");
                            controller.enqueue("b");
                            controller.close();
                        }
                    });
                    (async () => {
                        let out = "";
                        for await (const chunk of stream) out += chunk;
                        if (out !== "ab") throw new Error("Stream failed: " + out);
                        console.log("Streams test passed!");
                    })();
                `);
            )js");
        });

        // Run for a bit to let tests finish
        std::this_thread::sleep_for(std::chrono::seconds(2));
        w.terminate();
        w.run();
    } catch (const std::exception &e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
