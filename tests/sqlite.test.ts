import { expect, test, describe, beforeAll, afterAll } from "bun:test";
import { Database } from "bun:sqlite";

describe("SQLite Tests", () => {
  let db: Database;

  beforeAll(() => {
    db = new Database(":memory:");
    db.run("CREATE TABLE users (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT, email TEXT)");
  });

  afterAll(() => {
    db.close();
  });

  test("Insert and Query", () => {
    const insert = db.prepare("INSERT INTO users (name, email) VALUES (?, ?)");
    insert.run("Alice", "alice@example.com");
    insert.run("Bob", "bob@example.com");

    const users = db.query("SELECT * FROM users").all() as any[];
    expect(users).toHaveLength(2);
    expect(users[0].name).toBe("Alice");
    expect(users[1].name).toBe("Bob");
  });

  test("Update and Delete", () => {
    db.run("UPDATE users SET name = ? WHERE id = ?", ["Alice Smith", 1]);
    const alice = db.query("SELECT * FROM users WHERE id = 1").get() as any;
    expect(alice.name).toBe("Alice Smith");

    db.run("DELETE FROM users WHERE id = ?", [2]);
    const users = db.query("SELECT * FROM users").all();
    expect(users).toHaveLength(1);
  });

  test("Transactions", () => {
    const transaction = db.transaction((data) => {
      for (const item of data) {
        db.run("INSERT INTO users (name, email) VALUES (?, ?)", [item.name, item.email]);
      }
      return data.length;
    });

    const count = transaction([
      { name: "Charlie", email: "charlie@example.com" },
      { name: "Dave", email: "dave@example.com" },
    ]);

    expect(count).toBe(2);
    const users = db.query("SELECT * FROM users").all();
    expect(users).toHaveLength(3); // Alice + Charlie + Dave
  });

  test("Error Handling", () => {
    expect(() => {
      db.run("INSERT INTO non_existent_table (name) VALUES (?)", ["test"]);
    }).toThrow();
  });
});
