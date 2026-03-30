#include "webview/test_driver.hh"
#include "webview/detail/alloyscript_runtime.hh"
#include "alloy/api.h"
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

TEST_CASE("Alloy SQLite BigInt Round-tripping") {
    alloyscript_runtime runtime;
    auto db = runtime.sqlite_open(":memory:", false, true, false, true); // safe_integers = true
    REQUIRE(db != nullptr);

    sqlite3_exec(db->db, "CREATE TABLE test (v INTEGER)", nullptr, nullptr, nullptr);
    auto stmt_ins = runtime.sqlite_prepare(db, "INSERT INTO test (v) VALUES (?)", false);

    // Large 64-bit integer: 2^60
    std::string large_int = "1152921504606846976";
    runtime.sqlite_bind(stmt_ins, 1, large_int, "bigint");
    sqlite3_step(stmt_ins);

    auto stmt_sel = runtime.sqlite_prepare(db, "SELECT v FROM test", false);
    REQUIRE(sqlite3_step(stmt_sel) == SQLITE_ROW);

    // Check if it matches std::stoll conversion
    long long val = sqlite3_column_int64(stmt_sel, 0);
    REQUIRE(std::to_string(val) == large_int);
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

#ifdef WEBVIEW_PLATFORM_WINDOWS
TEST_CASE("Alloy GUI C API") {
    SECTION("Window creation and title") {
        auto win = alloy_create_window("Test Window", 800, 600);
        REQUIRE(win != nullptr);

        alloy_set_text(win, "New Title");
        char buf[256];
        alloy_get_text(win, buf, sizeof(buf));
        REQUIRE(std::string(buf) == "New Title");

        alloy_destroy(win);
    }

    SECTION("Button creation and parent-child") {
        auto win = alloy_create_window("Parent", 400, 300);
        auto btn = alloy_create_button(win);
        REQUIRE(btn != nullptr);

        alloy_set_text(btn, "Click");
        char buf[256];
        alloy_get_text(btn, buf, sizeof(buf));
        REQUIRE(std::string(buf) == "Click");

        alloy_destroy(win); // Should ideally destroy children too if implementation handles it,
                            // but here we just test lifecycle
        alloy_destroy(btn);
    }

    SECTION("ProgressBar values") {
        auto win = alloy_create_window("Progress", 400, 300);
        auto pb = alloy_create_progressbar(win);
        REQUIRE(pb != nullptr);

        alloy_set_value(pb, 0.75);
        REQUIRE(alloy_get_value(pb) == 0.75);

        alloy_destroy(pb);
        alloy_destroy(win);
    }
}
#endif
