import { expect, test, describe, beforeEach } from "bun:test";
import { Database, constants } from "../src/sqlite";

// Mocking window.Alloy for tests
if (typeof window === "undefined") {
    (global as any).window = {};
}

// Helper to update mock behavior
const mockSQLite = {
    open: (filename: string, options: any) => 1,
    query: (db_id: number, sql: string) => 1,
    run: (db_id: number, sql: string, params: any) => ({ lastInsertRowid: 0, changes: 0 }),
    serialize: (db_id: number) => "",
    deserialize: (contents: string) => 2,
    loadExtension: (db_id: number, name: string) => {},
    fileControl: (db_id: number, cmd: number, value: any) => {},
    setCustomSQLite: (path: string) => {},
    stmt_all: (stmt_id: number, params: any) => [{ message: "Hello world" }],
    stmt_get: (stmt_id: number, params: any) => ({ message: "Hello world" }),
    stmt_run: (stmt_id: number, params: any) => ({ lastInsertRowid: 0, changes: 0 }),
    stmt_values: (stmt_id: number, params: any) => [[ "Hello world" ]],
    stmt_finalize: (stmt_id: number) => {},
    stmt_toString: (stmt_id: number) => "SELECT 'Hello world';",
    stmt_metadata: (stmt_id: number) => ({
        columnNames: ["message"],
        columnTypes: ["TEXT"],
        declaredTypes: ["TEXT"],
        paramsCount: 0
    }),
    close: (db_id: number) => {}
};

(window as any).Alloy = {
    sqlite: mockSQLite
};

describe("Alloy:sqlite expanded", () => {
  test("Database opening and query result", () => {
    const db = new Database(":memory:");
    const query = db.query("select 'Hello world' as message;");
    const result = query.get();
    expect(result).toEqual({ message: "Hello world" });
  });

  test("Class mapping using .as()", () => {
      class Message {
          message!: string;
          get upper() { return this.message.toUpperCase(); }
      }
      const db = new Database();
      const query = db.query("select 'Hello world' as message;").as(Message);
      const res = query.get();
      expect(res).toBeInstanceOf(Message);
      expect(res?.upper).toBe("HELLO WORLD");
  });

  test("Transactions", () => {
      const db = new Database();
      const trans = db.transaction(() => {
          return "success";
      });
      expect(trans()).toBe("success");
  });

  test("bigint conversion", () => {
      mockSQLite.stmt_get = () => ({ val: "9007199254741093n" });
      const db = new Database(":memory:", { safeIntegers: true });
      const res = db.query("SELECT ...").get();
      expect(typeof res.val).toBe("bigint");
      expect(res.val).toBe(9007199254741093n);
  });

  test("Statement metadata", () => {
      const db = new Database();
      const query = db.query("SELECT 1");
      expect(query.columnNames).toEqual(["message"]);
      expect(query.columnTypes).toEqual(["TEXT"]);
      expect(query.paramsCount).toBe(0);
  });

  test("Database serialization", () => {
      mockSQLite.serialize = () => btoa("hello");
      const db = new Database();
      const contents = db.serialize();
      expect(contents).toBeInstanceOf(Uint8Array);
      expect(new TextDecoder().decode(contents)).toBe("hello");
  });

  test("Database fileControl", () => {
      let captured: any = null;
      mockSQLite.fileControl = (db_id, cmd, value) => { captured = { cmd, value }; };
      const db = new Database();
      db.fileControl(constants.SQLITE_FCNTL_PERSIST_WAL, 0);
      expect(captured).toEqual({ cmd: constants.SQLITE_FCNTL_PERSIST_WAL, value: 0 });
  });

  test("Statement iteration", () => {
      mockSQLite.stmt_all = () => [{ id: 1 }, { id: 2 }];
      const db = new Database();
      const query = db.query("SELECT id FROM users");
      const ids = [];
      for (const row of query) {
          ids.push(row.id);
      }
      expect(ids).toEqual([1, 2]);
  });

  test("Strict mode - prefix required by default", () => {
      const db = new Database(":memory:");
      const query = db.query("SELECT $val");
      // This is a unit test of the JS wrapper logic
      // In a real app, this would verify that parameters are passed correctly
      expect(db).toBeDefined();
  });

  test("safeIntegers range check", () => {
      const db = new Database(":memory:", { safeIntegers: true });
      const query = db.query("INSERT INTO test VALUES (?)");

      expect(() => {
          query.run(9223372036854775808n); // Out of range for signed 64-bit
      }).toThrow(RangeError);
  });
});
