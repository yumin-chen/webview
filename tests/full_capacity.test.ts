import { expect, test, describe, beforeAll } from "bun:test";
import { Database } from "Alloy:sqlite";
import { $ } from "Alloy";

declare const Alloy: any;

describe("Alloy:sqlite", () => {
  let db: Database;

  beforeAll(() => {
    db = new Database(":memory:");
  });

  test("prepared statements and parameters", () => {
    db.run("CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT)");
    const insert = db.prepare("INSERT INTO users (name) VALUES (?)");
    insert.run("Alice");
    insert.run("Bob");

    const users = db.query("SELECT * FROM users ORDER BY name ASC").all();
    expect(users).toHaveLength(2);
    expect(users[0].name).toBe("Alice");
    expect(users[1].name).toBe("Bob");
  });

  test("transactions", () => {
    db.run("CREATE TABLE accounts (balance INTEGER)");
    const trans = db.transaction((args) => {
      db.run("INSERT INTO accounts VALUES (100)");
      db.run("INSERT INTO accounts VALUES (200)");
    });
    trans();
    const count = db.query("SELECT COUNT(*) as c FROM accounts").get().c;
    expect(count).toBe(2);
  });

  test("serialization", () => {
    const data = db.serialize();
    expect(data).toBeInstanceOf(Uint8Array);
    expect(data.length).toBeGreaterThan(0);
  });
});

describe("Alloy Shell ($)", () => {
  test("text output", async () => {
    const out = await $`echo "hello"`.text();
    expect(out.trim()).toBe("hello");
  });

  test("piping", async () => {
    const count = await $`echo "one two three" | wc -w`.text();
    expect(count.trim()).toBe("3");
  });

  test("quoted arguments", async () => {
    const out = await $`echo "hello world"`.text();
    expect(out.trim()).toBe("hello world");
  });
});

describe("Alloy Cron", () => {
  test("lifecycle", async () => {
    const title = "test-job-" + Date.now();
    const res = await Alloy.cron("./test.ts", "*/5 * * * *", title);
    expect(res).toBe(true);
    const removed = await Alloy.cron.remove(title);
    expect(removed).toBe(true);
  });
});
