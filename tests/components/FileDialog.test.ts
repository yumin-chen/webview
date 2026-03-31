import { expect, test } from "bun:test";
import { FileDialog } from "../../src/gui/components/FileDialog";
test("FileDialog representation", () => {
  const comp = FileDialog({} as any);
  expect(comp.type).toBe("FileDialog");
});
