import { expect, test } from "bun:test";
import { Chip } from "../../src/gui/components/Chip";
test("Chip representation", () => {
  const comp = Chip({} as any);
  expect(comp.type).toBe("Chip");
});
