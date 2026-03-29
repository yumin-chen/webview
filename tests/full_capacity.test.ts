import { expect, test, describe } from "bun:test";
import { Database } from "Alloy:sqlite";
import { $ } from "Alloy";

declare const Alloy: any;

describe("Alloy:sqlite", () => {
  test("in-memory database", () => {
    const db = new Database(":memory:");
    db.run("CREATE TABLE test (id INTEGER PRIMARY KEY, msg TEXT)");
    db.run("INSERT INTO test (msg) VALUES ('hello')");

    const row = db.query("SELECT * FROM test").get();
    expect(row.msg).toBe("hello");
  });

  test("transactions", () => {
    const db = new Database(":memory:");
    db.run("CREATE TABLE test (msg TEXT)");
    const insert = db.transaction((msg) => {
      db.run("INSERT INTO test (msg) VALUES (?)", msg);
    });
    insert("task1");
    expect(db.query("SELECT COUNT(*) as cnt FROM test").get().cnt).toBe(1);
  });
});

describe("Alloy Shell", () => {
  test("piping and text", async () => {
    const result = await $`echo "hello world" | wc -w`.text();
    expect(result.trim()).toBe("2");
  });

  test("command substitution", async () => {
    const result = await $`echo $(echo hi)`.text();
    expect(result.trim()).toBe("hi");
  });
});

describe("Alloy Cron", () => {
  test("parse expression", () => {
    const next = Alloy.cron.parse("0 0 * * *");
    expect(next).toBeInstanceOf(Date);
  });
});
