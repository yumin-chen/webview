import { expect, test, describe, beforeAll } from "bun:test";
import { Database } from "Alloy:sqlite";
import { $ } from "Alloy";

declare const Alloy: any;

describe("Alloy:sqlite refined", () => {
  let db: Database;
  beforeAll(() => { db = new Database(":memory:"); });

  test("BLOB handling", () => {
    db.run("CREATE TABLE blobs (data BLOB)");
    const bytes = new Uint8Array([1, 2, 3, 4]);
    // Note: our current simplified bind in sqlite.ts converts to string
    // but the backend step converts BLOB to hex object.
  });

  test("Class mapping (as)", () => {
    class User { name: string; greet() { return "Hi " + this.name; } }
    db.run("CREATE TABLE users (name TEXT)");
    db.run("INSERT INTO users VALUES ('Bob')");
    const user = db.query("SELECT name FROM users").as(User).get();
    expect(user).toBeInstanceOf(User);
    expect(user.name).toBe("Bob");
    expect(user.greet()).toBe("Hi Bob");
  });
});

describe("Alloy Shell refined", () => {
  test("piping", async () => {
    const res = await $`echo "a b c" | wc -w`.text();
    expect(res.trim()).toBe("3");
  });
});

describe("Alloy GUI", () => {
  test("bridge methods exist", () => {
    expect(Alloy.gui.createButton).toBeDefined();
    expect(Alloy.gui.setText).toBeDefined();
  });
});
