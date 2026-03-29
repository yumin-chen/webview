import { expect, test, describe } from "bun:test";

describe("SQLite Bridge", () => {
    test("should open a database and execute queries", async () => {
        const db = new (window as any).alloy.sqlite.Database(":memory:");
        db.run("CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT)");
        db.run("INSERT INTO users (name) VALUES (?)", ["Alice"]);
        const row = db.query("SELECT * FROM users WHERE name = ?").get("Alice");
        expect(row).toBeDefined();
        expect(row.name).toBe("Alice");
        db.close();
    });
});
