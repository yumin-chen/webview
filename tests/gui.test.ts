import { expect, test, describe } from "bun:test";
import * as GUI from "../src/gui";

describe("Alloy:gui Component Unit Tests", () => {
  // Input Controls
  test("Button representation", () => {
    const btn = GUI.Button({ label: "Click", onClick: () => {} });
    expect(btn.type).toBe("Button");
    expect(btn.props.label).toBe("Click");
  });

  test("TextField representation", () => {
    const tf = GUI.TextField({ placeholder: "Type here" });
    expect(tf.type).toBe("TextField");
    expect(tf.props.placeholder).toBe("Type here");
  });

  test("Slider representation", () => {
      const slider = GUI.Slider({ value: 50, min: 0, max: 100 });
      expect(slider.type).toBe("Slider");
      expect(slider.props.value).toBe(50);
  });

  test("Switch representation", () => {
      const sw = GUI.Switch({ checked: true });
      expect(sw.type).toBe("Switch");
      expect(sw.props.checked).toBe(true);
  });

  // Display Components
  test("Label representation", () => {
      const lbl = GUI.Label({ text: "Status: OK" });
      expect(lbl.type).toBe("Label");
      expect(lbl.props.text).toBe("Status: OK");
  });

  test("ProgressBar representation", () => {
      const pb = GUI.ProgressBar({ value: 0.75 });
      expect(pb.type).toBe("ProgressBar");
      expect(pb.props.value).toBe(0.75);
  });

  test("Badge representation", () => {
      const badge = GUI.Badge({ text: "New" });
      expect(badge.type).toBe("Badge");
      expect(badge.props.text).toBe("New");
  });

  // Layout Containers
  test("VStack representation", () => {
      const stack = GUI.VStack({ spacing: 10, children: [] });
      expect(stack.type).toBe("VStack");
      expect(stack.props.spacing).toBe(10);
  });

  // Dialogs
  test("Dialog representation", () => {
      const dlg = GUI.Dialog({ title: "Warning", children: [] });
      expect(dlg.type).toBe("Dialog");
      expect(dlg.props.title).toBe("Warning");
  });

  // Additional
  test("WebView representation", () => {
      const wv = GUI.WebView({ src: "https://example.com" });
      expect(wv.type).toBe("WebView");
      expect(wv.props.src).toBe("https://example.com");
  });
});
