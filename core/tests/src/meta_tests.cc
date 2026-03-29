#include "webview/test_driver.hh"
#include "webview/webview.h"
#include "webview/meta.hh"
#include "webview/detail/meta_js.hh"
#include "webview/detail/base64.hh"

TEST_CASE("MetaScript: spawnSync with cwd") {
  webview::meta::SubprocessManager mgr(nullptr);
  auto res_json = mgr.spawnSync({"pwd"}, "{\"cwd\": \"/\"}");
  auto stdout_b64 = webview::detail::json_parse(res_json, "stdout", -1);
  auto out = webview::detail::base64_decode(stdout_b64);
  REQUIRE(out == "/\n");
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
