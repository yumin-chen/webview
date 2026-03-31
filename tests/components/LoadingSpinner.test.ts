import { expect, test } from "bun:test";
import { LoadingSpinner } from "../../src/gui/components/LoadingSpinner";
test("LoadingSpinner representation", () => {
  const comp = LoadingSpinner({});
  expect(comp.type).toBe("LoadingSpinner");
});
