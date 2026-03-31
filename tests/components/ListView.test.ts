import { expect, test } from "bun:test";
import { ListView } from "../../src/gui/components/ListView";
test("ListView representation", () => {
  const comp = ListView({} as any);
  expect(comp.type).toBe("ListView");
});
