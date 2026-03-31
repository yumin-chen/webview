import { expect, test } from "bun:test";
import { Link } from "../../src/gui/components/Link";
test("Link representation", () => {
  const comp = Link({} as any);
  expect(comp.type).toBe("Link");
});
