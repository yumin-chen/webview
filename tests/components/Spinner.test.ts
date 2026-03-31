import { expect, test } from "bun:test";
import { Spinner } from "../../src/gui/components/Spinner";
test("Spinner representation", () => {
  const comp = Spinner({} as any);
  expect(comp.type).toBe("Spinner");
});
