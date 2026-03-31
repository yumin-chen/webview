import { expect, test } from "bun:test";
import { Window } from "../../src/gui/components/Window";
test("Window representation", () => {
  const comp = Window({ title: "Test", children: [] });
  expect(comp.type).toBe("Window");
  expect(comp.props.title).toBe("Test");
});
