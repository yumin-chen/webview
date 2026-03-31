import { expect, test } from "bun:test";
import { RichTextEditor } from "../../src/gui/components/RichTextEditor";
test("RichTextEditor representation", () => {
  const comp = RichTextEditor({} as any);
  expect(comp.type).toBe("RichTextEditor");
});
