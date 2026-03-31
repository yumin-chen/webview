import { expect, test } from "bun:test";
import { CodeEditor } from "../../src/gui/components/CodeEditor";
test("CodeEditor representation", () => {
  const comp = CodeEditor({} as any);
  expect(comp.type).toBe("CodeEditor");
});
