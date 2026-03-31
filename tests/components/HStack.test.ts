import { expect, test } from "bun:test";
import { HStack } from "../../src/gui/components/HStack";
test("HStack representation", () => {
  const comp = HStack({} as any);
  expect(comp.type).toBe("HStack");
});
