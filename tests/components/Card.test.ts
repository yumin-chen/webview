import { expect, test } from "bun:test";
import { Card } from "../../src/gui/components/Card";
test("Card representation", () => {
  const comp = Card({} as any);
  expect(comp.type).toBe("Card");
});
