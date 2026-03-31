import { expect, test } from "bun:test";
import { GroupBox } from "../../src/gui/components/GroupBox";
test("GroupBox representation", () => {
  const comp = GroupBox({} as any);
  expect(comp.type).toBe("GroupBox");
});
