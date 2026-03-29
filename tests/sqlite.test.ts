import { expect, test, describe } from "bun:test";

describe("Alloy.sqlite", () => {
    test("should open an in-memory database and run queries", () => {
        const db = new Alloy.sqlite.Database(":memory:");
        db.run("CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT)");
        db.run("INSERT INTO users (name) VALUES (?)", ["Alice"]);
        db.run("INSERT INTO users (name) VALUES (?)", ["Bob"]);

        const stmt = db.prepare("SELECT * FROM users WHERE name = ?");
        const row = stmt.get("Alice");

        expect(row).toBeDefined();
        expect(row.name).toBe("Alice");

        const all = db.query("SELECT * FROM users ORDER BY name").all();
        expect(all).toHaveLength(2);
        expect(all[0].name).toBe("Alice");
        expect(all[1].name).toBe("Bob");

        db.close();
    });

    test("should handle numeric parameters", () => {
        const db = new Alloy.sqlite.Database(":memory:");
        db.run("CREATE TABLE vals (v REAL)");
        db.run("INSERT INTO vals VALUES (?)", [3.14]);

        const row = db.query("SELECT * FROM vals WHERE v > ?").get(3);
        expect(row.v).toBe(3.14);

        db.close();
    });
});
