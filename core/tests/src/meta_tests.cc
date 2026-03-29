#include "webview/test_driver.hh"
#include "webview/webview.h"
#include "webview/meta.hh"
#include "webview/detail/meta_js.hh"
#include "webview/detail/base64.hh"
#include <iostream>

TEST_CASE("MetaScript: cron.parse") {
  webview::webview w(true, nullptr);
  w.init(webview::detail::meta_js);

  w.bind("finish_test", [&](const std::string& req) -> std::string {
    auto res = webview::detail::json_parse(req, "", 0);
    // 2025-01-16T09:30:00.000Z is the next MON-FRI 9:30 after 2025-01-15 10:00
    if (res != "2025-01-16T09:30:00.000Z") {
        std::cerr << "ACTUAL CRON: " << res << "\n";
    }
    REQUIRE(res == "2025-01-16T09:30:00.000Z");
    w.terminate();
    return "";
  });

  w.set_html(R"html(
    <script>
      (async () => {
        try {
          const from = new Date(Date.UTC(2025, 0, 15, 10, 0, 0));
          const next = window.meta.cron.parse("30 9 * * MON-FRI", from);
          window.finish_test(next ? next.toISOString() : "null");
        } catch (e) {
          window.finish_test("ERROR: " + e.message);
        }
      })();
    </script>
  )html");
  w.run();
}

TEST_CASE("MetaScript: functional async spawn and base64 data") {
  auto w_ptr = std::make_shared<webview::webview>(true, nullptr);
  auto mgr = std::make_shared<webview::meta::SubprocessManager>(w_ptr.get());
  mgr->bind(*w_ptr);

  w_ptr->bind("finish_test", [w_ptr](const std::string& req) -> std::string {
    auto out = webview::detail::json_parse(req, "", 0);
    REQUIRE(out == "hello\n");
    w_ptr->terminate();
    return "";
  });

  w_ptr->init(webview::detail::meta_js);
  w_ptr->set_html(R"html(
    <script>
      (async () => {
        try {
          const proc = window.meta.spawn(['echo', 'hello']);
          const out = await proc.stdout.text();
          window.finish_test(out);
        } catch (e) {
          window.finish_test("ERROR: " + e.message);
        }
      })();
    </script>
  )html");
  w_ptr->run();
}
