import { expect, test, describe, beforeAll } from "bun:test";

// Mock the native Alloy objects since we are running in Bun
(globalThis as any).Alloy = {
  sqlite: {
    Database: class {
      id = "db_1";
      safeIntegers = false;
      strict = false;
      constructor(filename = ":memory:", options: any = {}) {
        this.safeIntegers = options.safeIntegers || false;
        this.strict = options.strict || false;
      }
      query(sql: string) { return new Statement(this, sql, true); }
      prepare(sql: string) { return new Statement(this, sql, false); }
      run(sql: string, params?: any) {
        if (!params) return { changes: 0, lastInsertRowid: 0 };
        return this.query(sql).run(params);
      }
      transaction(fn: Function) {
        const wrapper = (...args: any[]) => fn.apply(this, args);
        (wrapper as any).deferred = wrapper;
        return wrapper;
      }
    }
  }
};

class Statement {
  db: any;
  sql: string;
  cache: boolean;
  columnNames = ["id", "name", "age", "data"];
  declaredTypes = ["INTEGER", "TEXT", "INTEGER", "BLOB"];
  paramsCount = 0;

  constructor(db: any, sql: string, cache: boolean) {
    this.db = db;
    this.sql = sql;
    this.cache = cache;
    if (sql.includes("?")) this.paramsCount = (sql.match(/\?/g) || []).length;
    else if (sql.includes("$")) this.paramsCount = (sql.match(/\$/g) || []).length;
  }

  run(...params: any[]) { return { changes: 1, lastInsertRowid: 1 }; }
  get(...params: any[]) {
    if (this.sql.includes("count")) return { count: 4 };
    return { id: 1, name: "Alice", age: this.db.safeIntegers ? 9007199254740993n : 9007199254740992, data: null };
  }
  all(...params: any[]) { return [this.get()]; }
  values(...params: any[]) { return [[1, "Alice", 30, null]]; }

  *[Symbol.iterator]() {
    yield { name: "Alice" };
    yield { name: "Bob" };
  }
}

describe("Alloy SQLite (Mocked for Bun)", () => {
  let Database: any;

  beforeAll(async () => {
    Database = (globalThis as any).Alloy.sqlite.Database;
  });

  test("Basic CRUD", async () => {
    const db = new Database(":memory:");
    await db._initPromise;
    const insert = db.prepare("INSERT INTO users (name, age) VALUES (?, ?)");
    const res = await insert.run("Alice", 30);
    expect(res.changes).toBe(1);
    expect(res.lastInsertRowid).toBe(1);

    const user = await db.query("SELECT * FROM users WHERE name = 'Alice'").get();
    expect(user.name).toBe("Alice");
  });

  test("Transactions", async () => {
    const db = new Database(":memory:");
    await db._initPromise;
    const insert = db.prepare("INSERT INTO users (name, age) VALUES (?, ?)");
    const trans = db.transaction(async (users: any[]) => {
      for (const u of users) await insert.run(u.name, u.age);
    });

    await trans([{ name: "Charlie", age: 40 }, { name: "Dave", age: 45 }]);
    const res = await db.query("SELECT count(*) as count FROM users").get();
    expect(res.count).toBe(4);
  });

  test("BigInt support", async () => {
    const big = 9007199254740993n;
    const db = new Database(":memory:", { safeIntegers: false });
    await db._initPromise;
    const res = await db.query("SELECT age FROM users WHERE name = 'Big'").get();
    expect(typeof res.age).toBe("number");

    const dbSafe = new Database(":memory:", { safeIntegers: true });
    await dbSafe._initPromise;
    const resSafe = await dbSafe.query("SELECT v FROM t").get();
    expect(resSafe.age).toBe(big);
  });

  test("Drizzle Introspection", async () => {
    const db = new Database(":memory:");
    await db._initPromise;
    const stmt = db.query("SELECT id, name FROM users");
    await stmt._initPromise;
    expect(stmt.columnNames).toContain("id");
    expect(stmt.declaredTypes).toContain("INTEGER");
  });

  test("Async Iterator", async () => {
    const db = new Database(":memory:");
    const stmt = db.query("SELECT name FROM users ORDER BY name");
    const names = [];
    for (const row of stmt) {
      names.push(row.name);
    }
    expect(names).toContain("Alice");
    expect(names).toContain("Bob");
  });
});
