import { expect, test } from "bun:test";
import { TabView } from "../../src/gui/components/TabView";
test("TabView representation", () => {
  const comp = TabView({} as any);
  expect(comp.type).toBe("TabView");
});
