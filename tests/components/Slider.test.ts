import { expect, test } from "bun:test";
import { Slider } from "../../src/gui/components/Slider";
test("Slider representation", () => {
  const comp = Slider({} as any);
  expect(comp.type).toBe("Slider");
});
