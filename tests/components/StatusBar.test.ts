import { expect, test } from "bun:test";
import { StatusBar } from "../../src/gui/components/StatusBar";
test("StatusBar representation", () => {
  const comp = StatusBar({} as any);
  expect(comp.type).toBe("StatusBar");
});
