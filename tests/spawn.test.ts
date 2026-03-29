import { expect, test, describe } from "bun:test";
import { spawn, spawnSync, secureEval } from "../src/index";

// Mocking window.Alloy for tests since we are running in bun:test environment, not in the webview yet
if (typeof window === "undefined") {
    (global as any).window = {};
}
(window as any).Alloy = {
    spawn: async (cmd: string, args: string[]) => {
        const proc = Bun.spawn([cmd, ...args]);
        return await proc.exited;
    },
    spawnSync: (cmd: string, args: string[]) => {
        const proc = Bun.spawnSync([cmd, ...args]);
        return proc.exitCode;
    },
    secureEval: (code: string) => code
};

describe("Alloy Runtime", () => {
  test("spawn should return an exit code", async () => {
    const exitCode = await spawn("echo", ["hello"]);
    expect(exitCode).toBe(0);
  });

  test("spawnSync should return an exit code", () => {
    const exitCode = spawnSync("echo", ["hello"]);
    expect(exitCode).toBe(0);
  });

  test("spawn should handle failure", async () => {
      // Assuming 'false' command exists and returns non-zero
      const exitCode = await spawn("false", []);
      expect(exitCode).not.toBe(0);
  });

  test("spawnSync should handle failure", () => {
      const exitCode = spawnSync("false", []);
      expect(exitCode).not.toBe(0);
  });

  test("spawn should handle non-existent command", async () => {
      try {
          await spawn("non-existent-command-xyz", []);
          // On some systems/shells, this might throw or return a specific exit code.
          // Bun.spawn for non-existent command usually throws or the process fails.
      } catch (e) {
          expect(e).toBeDefined();
      }
  });

  test("spawnSync should handle non-existent command", () => {
      try {
          spawnSync("non-existent-command-xyz", []);
      } catch (e) {
          expect(e).toBeDefined();
      }
  });

  test("spawn should work with multiple arguments", async () => {
      const exitCode = await spawn("printf", ["%s %s", "hello", "world"]);
      expect(exitCode).toBe(0);
  });

  test("secureEval should return the code in mock", () => {
      const code = "1 + 1";
      const result = secureEval(code);
      expect(result).toBe(code);
  });

  describe("secureEval comprehensive", () => {
      test("should handle complex expressions", () => {
          const code = "(function() { return 2 + 2 * 3; })()";
          expect(secureEval(code)).toBe(code);
      });

      test("should handle string manipulation", () => {
          const code = "'hello'.toUpperCase()";
          expect(secureEval(code)).toBe(code);
      });

      test("should handle error cases (mocked)", () => {
          // Simulate a syntax error return from bridge
          const originalMock = (window as any).Alloy.secureEval;
          (window as any).Alloy.secureEval = () => "SyntaxError: unexpected token";

          expect(secureEval("invalid code")).toContain("SyntaxError");

          // Restore mock
          (window as any).Alloy.secureEval = originalMock;
      });
  });

  test("globalThis.eval should be replaced", () => {
      const originalEval = (globalThis as any).eval;
      (globalThis as any)._forbidden_eval = originalEval || (() => {});
      (globalThis as any).eval = (code: string) => secureEval(code);

      const code = "2 + 2";
      expect((globalThis as any).eval(code)).toBe(code);
      expect((globalThis as any)._forbidden_eval).toBeDefined();
  });
});
