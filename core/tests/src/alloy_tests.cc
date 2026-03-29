#include "webview/test_driver.hh"
#include "webview/detail/alloyscript_runtime.hh"
#include <fstream>
#include <chrono>
#include <thread>
#include <iostream>

using namespace webview::detail;

TEST_CASE("Alloy Tokenizer") {
    auto tokens = alloyscript_runtime::tokenize("echo \"hello world\" 'single quotes' escaped\\ space");
    REQUIRE(tokens.size() == 4);
    REQUIRE(tokens[0] == "echo");
    REQUIRE(tokens[1] == "hello world");
    REQUIRE(tokens[2] == "single quotes");
    REQUIRE(tokens[3] == "escaped space");
}

TEST_CASE("Alloy spawnSync") {
    alloyscript_runtime runtime;

    SECTION("Successful command") {
        std::vector<std::string> args = {"echo", "test-output"};
        std::string result_json = runtime.spawnSync(args);
        REQUIRE(result_json.find("\"success\": true") != std::string::npos);
        REQUIRE(result_json.find("test-output") != std::string::npos);
    }

    SECTION("Failed command") {
        std::vector<std::string> args = {"false"};
        std::string result_json = runtime.spawnSync(args);
        REQUIRE(result_json.find("\"success\": false") != std::string::npos);
    }

    SECTION("Stderr capture") {
        std::vector<std::string> args = {"sh", "-c", "echo sync-error >&2"};
        std::string result_json = runtime.spawnSync(args);
        REQUIRE(result_json.find("sync-error") != std::string::npos);
    }
}

TEST_CASE("Alloy spawn (Async)") {
    alloyscript_runtime runtime;

    SECTION("Environment variables") {
        std::map<std::string, std::string> env = {{"FOO", "BAR"}};
        // Use full path for printenv to avoid PATH issues with execve
        std::vector<std::string> args = {"/usr/bin/printenv", "FOO"};
        auto state = runtime.spawn(args, "", env);
        REQUIRE(state != nullptr);

        std::string stdout_data;
        std::atomic<bool> exited{false};

        state->on_stdout = [&](const std::string& data) { stdout_data += data; };
        state->on_exit = [&](int code, int sig) { (void)code; (void)sig; exited = true; };

        runtime.start_monitoring(state);
        int timeout = 500; // 5 seconds
        while (!exited && timeout > 0) { std::this_thread::sleep_for(std::chrono::milliseconds(10)); timeout--; }

        REQUIRE(exited == true);
        REQUIRE(stdout_data.find("BAR") != std::string::npos);
    }

    SECTION("Working directory") {
        std::vector<std::string> args = {"/usr/bin/pwd"};
        auto state = runtime.spawn(args, "/tmp");
        REQUIRE(state != nullptr);

        std::string stdout_data;
        std::atomic<bool> exited{false};

        state->on_stdout = [&](const std::string& data) { stdout_data += data; };
        state->on_exit = [&](int code, int sig) { (void)code; (void)sig; exited = true; };

        runtime.start_monitoring(state);
        int timeout = 500;
        while (!exited && timeout > 0) { std::this_thread::sleep_for(std::chrono::milliseconds(10)); timeout--; }

        REQUIRE(exited == true);
        REQUIRE(stdout_data.find("tmp") != std::string::npos);
    }
}

TEST_CASE("Alloy Shell Pipelines") {
    alloyscript_runtime runtime;

#ifndef _WIN32
    SECTION("Basic pipe") {
        auto res = runtime.shell("echo 'hello world' | wc -w");
        REQUIRE(res.exit_code == 0);
        REQUIRE(res.stdout_data.find("2") != std::string::npos);
    }

    SECTION("Complex pipeline") {
        auto res = runtime.shell("printf 'a\\nb\\nc\\n' | grep b | wc -l");
        REQUIRE(res.exit_code == 0);
        REQUIRE(res.stdout_data.find("1") != std::string::npos);
    }
#endif
}
