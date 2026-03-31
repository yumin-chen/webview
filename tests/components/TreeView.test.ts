import { expect, test } from "bun:test";
import { TreeView } from "../../src/gui/components/TreeView";
test("TreeView representation", () => {
  const comp = TreeView({} as any);
  expect(comp.type).toBe("TreeView");
});
