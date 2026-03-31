import { expect, test } from "bun:test";
import { WebView } from "../../src/gui/components/WebView";
test("WebView representation", () => {
  const comp = WebView({} as any);
  expect(comp.type).toBe("WebView");
});
