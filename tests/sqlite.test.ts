import { expect, test, describe, beforeEach } from "bun:test";
import { Database } from "../src/sqlite";

// Mocking window.Alloy for tests
if (typeof window === "undefined") {
    (global as any).window = {};
}
(window as any).Alloy = {
    sqlite: {
        open: (filename: string, options: any) => 1,
        query: (db_id: number, sql: string) => 1,
        run: (db_id: number, sql: string, params: any) => ({ lastInsertRowid: 0, changes: 0 }),
        stmt_all: (stmt_id: number, params: any) => {
            if (stmt_id === 1) return [{ message: "Hello world" }];
            return [];
        },
        stmt_get: (stmt_id: number, params: any) => {
            if (stmt_id === 1) return { message: "Hello world" };
            return null;
        },
        stmt_run: (stmt_id: number, params: any) => ({ lastInsertRowid: 0, changes: 0 }),
        stmt_values: (stmt_id: number, params: any) => [[ "Hello world" ]],
        stmt_finalize: (stmt_id: number) => {},
        stmt_toString: (stmt_id: number) => "SELECT 'Hello world';",
        close: (db_id: number) => {}
    }
};

describe("Alloy:sqlite", () => {
  test("Database opening and query result", () => {
    const db = new Database(":memory:");
    const query = db.query("select 'Hello world' as message;");
    const result = query.get();
    expect(result).toEqual({ message: "Hello world" });
  });

  test("Database.all() result", () => {
    const db = new Database();
    const query = db.query("select 'Hello world' as message;");
    const results = query.all();
    expect(results).toEqual([{ message: "Hello world" }]);
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
      // Mock returning bigint string from bridge
      (window as any).Alloy.sqlite.stmt_get = () => ({ val: "9007199254741093n" });
      const db = new Database();
      const res = db.query("SELECT ...").get();
      expect(typeof res.val).toBe("bigint");
      expect(res.val).toBe(9007199254741093n);
  });
});
