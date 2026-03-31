import { expect, test } from "bun:test";
import { TextArea } from "../../src/gui/components/TextArea";
test("TextArea representation", () => {
  const comp = TextArea({} as any);
  expect(comp.type).toBe("TextArea");
});
