import { expect, test } from "bun:test";
import { Image } from "../../src/gui/components/Image";
test("Image representation", () => {
  const comp = Image({} as any);
  expect(comp.type).toBe("Image");
});
