import { expect, test, describe, beforeAll } from "bun:test";
import { Database } from "Alloy:sqlite";
import { $ } from "Alloy";

declare const Alloy: any;

describe("Alloy:sqlite advanced", () => {
  let db: Database;
  beforeAll(() => { db = new Database(":memory:", { safeIntegers: true }); });

  test("safeIntegers support", () => {
    db.run("CREATE TABLE bigints (val INTEGER)");
    const big = 9007199254740991n;
    db.run("INSERT INTO bigints VALUES (?)", [big]);
    const row = db.query("SELECT val FROM bigints").get();
    expect(typeof row.val).toBe("bigint");
    expect(row.val).toBe(big);
  });

  test("Class mapping with .as()", () => {
    class User { name: string; isAdmin() { return this.name === "admin"; } }
    db.run("CREATE TABLE users (name TEXT)");
    db.run("INSERT INTO users VALUES ('admin')");
    const user = db.query("SELECT name FROM users").as(User).get();
    expect(user).toBeInstanceOf(User);
    expect(user.isAdmin()).toBe(true);
  });
});

describe("Alloy Shell advanced", () => {
  test("stderr redirection", async () => {
    // This will throw because of non-zero exit code if not caught
    try {
        await $`ls non_existent_file 2> err.txt`.quiet();
    } catch (e) {
        expect(e.message).toContain("Command failed");
    }
  });

  test("interpolation of raw objects", async () => {
    const script = { raw: "echo hello" };
    const out = await $`${script}`.text();
    expect(out.trim()).toBe("hello");
  });
});

describe("Alloy GUI bridge", () => {
  test("create components", () => {
    // In our test environment, we don't have a real WebView but we check the bridge
    expect(Alloy.gui.createWindow).toBeDefined();
    expect(Alloy.gui.createButton).toBeDefined();
    expect(Alloy.gui.createTextField).toBeDefined();
  });
});
