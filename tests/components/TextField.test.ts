import { expect, test, describe } from "bun:test";
import { TextField } from "../../src/gui/components/TextField";

describe("TextField Component", () => {
  test("TextField representation and props", () => {
    const comp = TextField({ placeholder: "Enter name", text: "John" });
    expect(comp.type).toBe("TextField");
    expect(comp.props.placeholder).toBe("Enter name");
    expect(comp.props.text).toBe("John");
  });

  test("TextField event simulation (onChange)", () => {
    let changedText = "";
    const comp = TextField({
        onChange: (text) => { changedText = text; }
    });

    if (comp.props.onChange) comp.props.onChange("New text");
    expect(changedText).toBe("New text");
  });
});
