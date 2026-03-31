import { expect, test } from "bun:test";
import { TextField } from "../../src/gui/components/TextField";
test("TextField representation", () => {
  const comp = TextField({ placeholder: "Type" });
  expect(comp.type).toBe("TextField");
  expect(comp.props.placeholder).toBe("Type");
});
