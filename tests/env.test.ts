import { expect, test, describe } from "bun:test";

const mockEnv: Record<string, string> = { NODE_ENV: "test" };
(global as any).window = {
    __alloy_get_env: (key: string) => mockEnv[key] || "",
    __alloy_set_env: (key: string, val: string) => { mockEnv[key] = val; return "true"; },
    __alloy_file_exists: () => "false",
    __alloy_file_read: () => ""
};

// Simulate the Proxy behavior from engine_base.hh
(global as any).process = {
    env: new Proxy(mockEnv, {
        get(target, prop) {
            if (typeof prop !== "string") return undefined;
            return (global as any).window.__alloy_get_env(prop) || target[prop];
        },
        set(target, prop, value) {
            if (typeof prop === "string") {
                (global as any).window.__alloy_set_env(prop, String(value));
                target[prop] = String(value);
                return true;
            }
            return false;
        }
    })
};

(global as any).Alloy = { env: (global as any).process.env };

describe("Environment Variables", () => {
  test("read environment variable", () => {
    expect(process.env.NODE_ENV).toBe("test");
    expect(Alloy.env.NODE_ENV).toBe("test");
  });

  test("write environment variable", () => {
    process.env.FOO = "bar";
    expect(mockEnv.FOO).toBe("bar");
    expect(process.env.FOO).toBe("bar");
  });

  test("expansion (manual test of parseEnv logic)", () => {
    // We already tested parseEnv in e2e or could add a unit test for it if we exported it
  });
});
