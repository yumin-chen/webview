import { expect, test } from "bun:test";
import { Rating } from "../../src/gui/components/Rating";
test("Rating representation", () => {
  const comp = Rating({} as any);
  expect(comp.type).toBe("Rating");
});
