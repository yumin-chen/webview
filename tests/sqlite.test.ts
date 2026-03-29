import { expect, test, describe, beforeAll } from "bun:test";
// We simulate the Alloy bridge for testing in Bun environment if needed,
// but here we draft the tests as they would run in the Alloy environment.
// In Alloy environment, these would be:
// import { Database } from "Alloy:sqlite";

describe("Alloy SQLite", () => {
  // Mocking the Database for Bun environment testing if not running in Alloy
  // In real Alloy usage, this would be the actual native binding.
  let Database: any;

  beforeAll(async () => {
    try {
        // @ts-ignore
        const mod = await import("Alloy:sqlite");
        Database = mod.Database;
    } catch (e) {
        console.warn("Alloy:sqlite not found, using mock for tests");
        // Simple mock to allow tests to be 'drafted' and syntactically checked by Bun
        Database = class MockDatabase {
            constructor(public filename: string = ":memory:", public options: any = {}) {}
            query(sql: string) { return { get: () => ({}), all: () => [], run: () => ({}), values: () => [], as: () => this.query(sql) }; }
            run(sql: string, params?: any) { return { lastInsertRowid: 0, changes: 0 }; }
            close() {}
            transaction(fn: any) { return fn; }
            static open(f: string, o: any) { return new MockDatabase(f, o); }
        };
    }
  });

  test("should open a database", () => {
    const db = new Database(":memory:");
    expect(db).toBeDefined();
    db.close();
  });

  test("should execute a simple query", () => {
    const db = new Database(":memory:");
    const result = db.query("SELECT 'hello' as msg").get();
    expect(result).toEqual({ msg: "hello" });
  });

  test("should handle parameters", () => {
    const db = new Database(":memory:");
    const result = db.query("SELECT ? as val").get("world");
    expect(result).toEqual({ val: "world" });
  });

  test("should handle BigInt", () => {
    const db = new Database(":memory:", { safeIntegers: true });
    const big = 9007199254740992n;
    const result = db.query("SELECT ? as val").get(big);
    expect(result.val).toBe(big);
  });

  test("should handle BLOB as Uint8Array", () => {
    const db = new Database(":memory:");
    const data = new Uint8Array([1, 2, 3]);
    db.run("CREATE TABLE blobs (d BLOB)");
    db.run("INSERT INTO blobs VALUES (?)", data);
    const result = db.query("SELECT d FROM blobs").get();
    expect(result.d).toBeInstanceOf(Uint8Array);
    expect(Array.from(result.d)).toEqual([1, 2, 3]);
  });

  test("should work with transactions", () => {
    const db = new Database(":memory:");
    db.run("CREATE TABLE test (id INTEGER)");

    const insert = db.transaction((id: number) => {
        db.run("INSERT INTO test VALUES (?)", id);
    });

    insert(1);
    const row = db.query("SELECT * FROM test").get();
    expect(row.id).toBe(1);
  });

  test("should support lazy iteration", () => {
    const db = new Database(":memory:");
    db.run("CREATE TABLE nums (n INTEGER)");
    for (let i = 0; i < 5; i++) db.run("INSERT INTO nums VALUES (?)", i);

    const results = [];
    for (const row of db.query("SELECT n FROM nums").iterate()) {
        results.push(row.n);
    }
    expect(results).toEqual([0, 1, 2, 3, 4]);
  });

  test("should work with Drizzle-style bindings", async () => {
    // Drizzle uses .all(), .get(), .run(), .values()
    const db = Database.open(":memory:");
    db.run("CREATE TABLE drizzle (id INTEGER PRIMARY KEY, name TEXT)");

    const insertResult = db.run("INSERT INTO drizzle (name) VALUES (?)", "test");
    expect(insertResult.changes).toBeGreaterThan(0);

    const all = db.query("SELECT * FROM drizzle").all();
    expect(all.length).toBe(1);
    expect(all[0].name).toBe("test");

    const values = db.query("SELECT name FROM drizzle").values();
    expect(values).toEqual([["test"]]);
  });
});
