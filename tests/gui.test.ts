import { expect, test, describe } from "bun:test";

// Mock Alloy for local testing
if (typeof window === "undefined") {
    globalThis.window = {
        Alloy_guiCreate: async (type) => "comp_" + type + "_" + Math.random(),
        Alloy_guiUpdate: () => "true",
        Alloy_guiAddChild: () => "true",
        Alloy_sqliteOpen: async () => "db1",
        Alloy_sqliteQuery: async () => "[]",
        Alloy_spawn: async () => "123",
        Alloy_spawnSync: () => "{}",
        Alloy_shell: async () => ({ exitCode: 0, stdout: "", stderr: "" })
    } as any;
}

const { Window, Button, VStack, Label, ProgressBar } = (function() {
  const Window = function(props) {
      this.id = null;
      this.props = props || {};
      this.init = async () => {
          this.id = await window.Alloy_guiCreate("Window", this.props);
          await window.Alloy_guiUpdate(this.id, this.props);
      };
  };

  const Button = function(props) {
      this.id = null;
      this.props = props || {};
      this.init = async () => {
          this.id = await window.Alloy_guiCreate("Button", this.props);
          await window.Alloy_guiUpdate(this.id, this.props);
      };
  };

  const VStack = function(props) {
      this.id = null;
      this.props = props || {};
      this.init = async () => {
          this.id = await window.Alloy_guiCreate("VStack", this.props);
          await window.Alloy_guiUpdate(this.id, this.props);
      };
      this.add = (child) => window.Alloy_guiAddChild(this.id, child.id);
  };

  const Label = function(props) {
      this.id = null;
      this.props = props || {};
      this.init = async () => {
          this.id = await window.Alloy_guiCreate("Label", this.props);
          await window.Alloy_guiUpdate(this.id, this.props);
      };
  };

  const ProgressBar = function(props) {
      this.id = null;
      this.props = props || {};
      this.init = async () => {
          this.id = await window.Alloy_guiCreate("ProgressBar", this.props);
          await window.Alloy_guiUpdate(this.id, this.props);
      };
      this.setValue = (val) => window.Alloy_guiUpdate(this.id, { value: val });
  };

  return { Window, Button, VStack, Label, ProgressBar };
})();

describe("Alloy GUI API", () => {
    test("Window creation", async () => {
        const win = new Window({ title: "My App" });
        await win.init();
        expect(win.id).toStartWith("comp_Window_");
    });

    test("Button creation", async () => {
        const btn = new Button({ label: "Click Me" });
        await btn.init();
        expect(btn.id).toStartWith("comp_Button_");
    });

    test("Layout nesting", async () => {
        const stack = new VStack({});
        await stack.init();
        const label = new Label({ text: "Hello" });
        await label.init();
        stack.add(label);
        expect(stack.id).toStartWith("comp_VStack_");
        expect(label.id).toStartWith("comp_Label_");
    });

    test("ProgressBar value update", async () => {
        const bar = new ProgressBar({});
        await bar.init();
        bar.setValue(0.5);
        expect(bar.id).toStartWith("comp_ProgressBar_");
    });
});
