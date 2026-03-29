import { expect, test, describe } from "bun:test";

describe("Alloy.sqlite", () => {
    test("should open an in-memory database and run queries", () => {
        const db = new Alloy.sqlite.Database(":memory:");
        db.run("CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT)");
        db.run("INSERT INTO users (name) VALUES ('Alice')");
        db.run("INSERT INTO users (name) VALUES ('Bob')");

        const stmt = db.prepare("SELECT * FROM users ORDER BY name");
        const rows = stmt.all();

        expect(rows).toHaveLength(2);
        expect(rows[0].name).toBe("Alice");
        expect(rows[1].name).toBe("Bob");

        db.close();
    });

    test("should handle single row retrieval", () => {
        const db = new Alloy.sqlite.Database(":memory:");
        db.run("CREATE TABLE settings (key TEXT, value TEXT)");
        db.run("INSERT INTO settings VALUES ('theme', 'dark')");

        const row = db.query("SELECT * FROM settings WHERE key = 'theme'").get();
        expect(row.value).toBe("dark");

        db.close();
    });
});
