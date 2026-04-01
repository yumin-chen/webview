import { expect, test, describe } from "bun:test";

// Mocking window.Alloy for tests
const DatabaseMock = function(filename, options) {
    this.init = async () => {};
    this.query = (sql) => {
        return {
            all: async (params) => {
                if (sql.includes("movies")) return [{id: 1, title: "The Matrix"}];
                return [];
            },
            get: async (params) => {
                if (sql.includes("Hello world")) return { message: "Hello world" };
                return null;
            },
            run: async (params) => ({ lastInsertRowid: 1, changes: 1 }),
            values: async (params) => [ [1, "The Matrix"] ]
        };
    };
    this.run = (sql, params) => this.query(sql).run(params);
};

const AlloyMock = {
    $: (strings, ...values) => {
        const promise = Promise.resolve({
            text: () => Promise.resolve("test-output"),
            json: () => Promise.resolve({ status: "ok" }),
            exitCode: 0
        });
        promise.text = () => Promise.resolve("test-output");
        return promise;
    },
    sqlite: { Database: DatabaseMock }
};

describe("Alloy SQLite", () => {
    test("should open database and run simple query", async () => {
        const db = new AlloyMock.sqlite.Database(":memory:");
        await db.init();
        const res = await db.query("select 'Hello world' as message").get();
        expect(res.message).toBe("Hello world");
    });

    test("should fetch all rows", async () => {
        const db = new AlloyMock.sqlite.Database(":memory:");
        await db.init();
        const res = await db.query("select * from movies").all();
        expect(res.length).toBe(1);
        expect(res[0].title).toBe("The Matrix");
    });
});

describe("Alloy Shell", () => {
    test("should run echo", async () => {
        const res = await AlloyMock.$`echo hello`.text();
        expect(res).toBe("test-output");
    });
});
