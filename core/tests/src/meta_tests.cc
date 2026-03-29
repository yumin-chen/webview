#include "webview/test_driver.hh"
#include "webview/webview.h"
#include "webview/meta.hh"
#include "webview/detail/meta_js.hh"

TEST_CASE("MetaScript: spawnSync") {
  webview::meta::SubprocessManager mgr(nullptr);

  SECTION("echo hello") {
    auto res_json = mgr.spawnSync({"echo", "hello"}, "{}");
    auto success = webview::detail::json_parse(res_json, "success", -1);
    auto stdout_str = webview::detail::json_parse(res_json, "stdout", -1);
    REQUIRE(success == "true");
    REQUIRE(stdout_str == "hello\n");
  }

  SECTION("false") {
    auto res_json = mgr.spawnSync({"false"}, "{}");
    auto success = webview::detail::json_parse(res_json, "success", -1);
    auto exitCode = webview::detail::json_parse(res_json, "exitCode", -1);
    REQUIRE(success == "false");
    REQUIRE(exitCode == "1");
  }
}

TEST_CASE("MetaScript: spawn") {
  // Testing async spawn is harder without a full webview and event loop.
  // But we can at least check if it returns a PID.
  webview::meta::SubprocessManager mgr(nullptr);
  auto res_json = mgr.spawn({"ls"}, "{}");
  auto pid = webview::detail::json_parse(res_json, "pid", -1);
  REQUIRE(!pid.empty());
  REQUIRE(std::stoi(pid) > 0);
}
