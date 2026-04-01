import { expect, test, describe } from "bun:test";
import { Alloy } from "../src/index";

describe("Alloy REPL", () => {
  test("REPL instantiation and engine type", () => {
    const repl = new Alloy.REPL();
    expect(repl).toBeDefined();
  });
});
