import { expect, test } from "bun:test";
import { Icon } from "../../src/gui/components/Icon";
test("Icon representation", () => {
  const comp = Icon({} as any);
  expect(comp.type).toBe("Icon");
});
