import { expect, test } from "bun:test";
import { ContextMenu } from "../../src/gui/components/ContextMenu";
test("ContextMenu representation", () => {
  const comp = ContextMenu({} as any);
  expect(comp.type).toBe("ContextMenu");
});
