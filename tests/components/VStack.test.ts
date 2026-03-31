import { expect, test } from "bun:test";
import { VStack } from "../../src/gui/components/VStack";
test("VStack representation", () => {
  const comp = VStack({} as any);
  expect(comp.type).toBe("VStack");
});
