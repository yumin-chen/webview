import { expect, test } from "bun:test";
import { ProgressBar } from "../../src/gui/components/ProgressBar";
test("ProgressBar representation", () => {
  const comp = ProgressBar({} as any);
  expect(comp.type).toBe("ProgressBar");
});
