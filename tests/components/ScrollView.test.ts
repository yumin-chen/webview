import { expect, test } from "bun:test";
import { ScrollView } from "../../src/gui/components/ScrollView";
test("ScrollView representation", () => {
  const comp = ScrollView({} as any);
  expect(comp.type).toBe("ScrollView");
});
