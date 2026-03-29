#include "webview/test_driver.hh"
#include "webview/webview.h"
#include "webview/meta.hh"
#include "webview/detail/meta_js.hh"
#include "webview/detail/base64.hh"

namespace {
std::vector<std::string> parse_json_array(const std::string& json) {
    std::vector<std::string> result;
    for (int i = 0; ; ++i) {
        const char *value;
        size_t valuesz;
        if (webview::detail::json_parse_c(json.c_str(), json.length(), nullptr, i, &value, &valuesz) != 0) break;
        if (value[0] == '"') {
            int n = webview::detail::json_unescape(value, valuesz, nullptr);
            if (n >= 0) {
                char *decoded = new char[n + 1];
                webview::detail::json_unescape(value, valuesz, decoded);
                result.push_back(std::string(decoded, n));
                delete[] decoded;
            }
        } else result.push_back(std::string(value, valuesz));
    }
    return result;
}
}

TEST_CASE("MetaScript: spawnSync with cwd") {
  webview::meta::SubprocessManager mgr(nullptr);
  auto res_json = mgr.spawnSync({"pwd"}, "{\"cwd\": \"/\"}");
  auto stdout_b64 = webview::detail::json_parse(res_json, "stdout", -1);
  auto out = webview::detail::base64_decode(stdout_b64);
  REQUIRE(out == "/\n");
}

TEST_CASE("MetaScript: functional async spawn and binary-ish data") {
  auto w_ptr = std::make_shared<webview::webview>(true, nullptr);
  auto mgr = std::make_shared<webview::meta::SubprocessManager>(w_ptr.get());

  w_ptr->bind("__meta_spawn", [mgr, w_ptr](const std::string& id, const std::string& req, void*) {
    auto handle = webview::detail::json_parse(req, "", 0);
    auto cmd_json = webview::detail::json_parse(req, "", 1);
    auto opts_json = webview::detail::json_parse(req, "", 2);
    auto res = mgr->spawn(handle, parse_json_array(cmd_json), opts_json);
    w_ptr->resolve(id, 0, res);
  }, nullptr);

  w_ptr->bind("__meta_cleanup", [mgr](const std::string& req) -> std::string {
    mgr->cleanup(webview::detail::json_parse(req, "", 0));
    return "";
  });

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
