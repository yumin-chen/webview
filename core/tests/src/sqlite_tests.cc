#include "webview/test_driver.hh"
#include "webview/detail/sqlite_runtime.hh"
#include <chrono>
#include <thread>
#include <iostream>

using namespace webview::detail;

TEST_CASE("SQLite Runtime") {
    sqlite_runtime runtime;

    SECTION("Open in-memory database") {
        std::string id = runtime.open(":memory:", false, true, false, false);
        REQUIRE(!id.empty());
        auto db = runtime.get_db(id);
        REQUIRE(db != nullptr);
        runtime.close(id);
        REQUIRE(runtime.get_db(id) == nullptr);
    }

    SECTION("Basic Query Execution") {
        std::string id = runtime.open(":memory:", false, true, false, false);
        auto db = runtime.get_db(id);

        auto stmt = db->prepare("CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT)", false);
        std::string res = stmt->execute("run", false);
        REQUIRE(res.find("\"changes\":0") != std::string::npos);

        stmt = db->prepare("INSERT INTO users (name) VALUES ('Alice')", false);
        res = stmt->execute("run", false);
        REQUIRE(res.find("\"changes\":1") != std::string::npos);
        REQUIRE(res.find("\"lastInsertRowid\":1") != std::string::npos);

        stmt = db->prepare("SELECT * FROM users", false);
        res = stmt->execute("all", false);
        REQUIRE(res == "[{\"id\":1,\"name\":\"Alice\"}]");

        runtime.close(id);
    }

    SECTION("Parameter Binding") {
        std::string id = runtime.open(":memory:", false, true, false, false);
        auto db = runtime.get_db(id);

        db->prepare("CREATE TABLE test (val TEXT)", false)->execute("run", false);

        auto stmt = db->prepare("INSERT INTO test (val) VALUES (?)", true);
        stmt->bind("[\"Hello\"]", false);
        stmt->execute("run", false);

        stmt = db->prepare("SELECT val FROM test", false);
        std::string res = stmt->execute("get", false);
        REQUIRE(res == "{\"val\":\"Hello\"}");

        runtime.close(id);
    }

    SECTION("Safe Integers (BigInt)") {
        std::string id = runtime.open(":memory:", false, true, true, false);
        auto db = runtime.get_db(id);

        auto stmt = db->prepare("SELECT 9007199254740992 as big", false);
        std::string res = stmt->execute("get", true);
        REQUIRE(res == "{\"big\":\"9007199254740992n\"}");

        runtime.close(id);
    }

    SECTION("Strict Mode") {
        std::string id = runtime.open(":memory:", false, true, false, true);
        auto db = runtime.get_db(id);

        auto stmt = db->prepare("SELECT $val", false);
        REQUIRE_THROW(std::runtime_error, [&]() { stmt->bind("{\"wrong\":1}", true); });

        stmt->bind("{\"val\":123}", true);
        REQUIRE(stmt->execute("get", false) == "{\"$val\":123}");

        runtime.close(id);
    }

    SECTION("BigInt with 'n' suffix") {
        std::string id = runtime.open(":memory:", false, true, false, false);
        auto db = runtime.get_db(id);
        auto stmt = db->prepare("SELECT ?", false);
        stmt->bind("[\"1234567890123456789n\"]", false);
        // The output column name for SELECT ? might vary between sqlite versions,
        // it could be ?1 or ?
        std::string res = stmt->execute("get", false);
        REQUIRE(res.find("1234567890123456789") != std::string::npos);
        runtime.close(id);
    }

    SECTION("BLOB as Base64") {
        std::string id = runtime.open(":memory:", false, true, false, false);
        auto db = runtime.get_db(id);
        db->prepare("CREATE TABLE b (data BLOB)", false)->execute("run", false);

        // This is hard to test with the current bind_value as it doesn't handle base64 for bind
        // but we can test the output of a BLOB
        db->prepare("INSERT INTO b (data) VALUES (zeroblob(3))", false)->execute("run", false);
        std::string res = db->prepare("SELECT data FROM b", false)->execute("get", false);
        REQUIRE(res == "{\"data\":\"base64:AAAA\"}");
        runtime.close(id);
    }

    SECTION("Multi-query exec") {
        std::string id = runtime.open(":memory:", false, true, false, false);
        runtime.exec(id, "CREATE TABLE t1 (id); CREATE TABLE t2 (id);");
        auto db = runtime.get_db(id);
        REQUIRE(db->prepare("SELECT * FROM t1", false) != nullptr);
        REQUIRE(db->prepare("SELECT * FROM t2", false) != nullptr);
        runtime.close(id);
    }
}
