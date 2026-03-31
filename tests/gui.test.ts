import { expect, test, describe } from "bun:test";
import * as GUI from "../src/gui";

describe("Alloy:gui Logic Tests", () => {
  test("Styling and Color API", () => {
      const color = GUI.Color.blue(500);
      expect(color).toBe("blue-500");
  });
});
