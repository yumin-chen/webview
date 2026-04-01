import { Window, Button, VStack, HStack, TextField, Label, Alloy } from "../src/index";

// AlloyScript GUI Example (TypeScript)
const app = () => {
  const win = Window({
    title: "AlloyScript Native GUI",
    width: 800,
    height: 600,
  });

  const root = HStack({ parent: win });

  const sidebar = VStack({
    parent: root,
    flex: 0.3,
    padding: [10, 10, 10, 10],
  });

  const content = VStack({
    parent: root,
    flex: 0.7,
    padding: [20, 20, 20, 20],
  });

  Label({
    parent: sidebar,
    text: "NAVIGATION",
  });

  ["Dashboard", "Settings", "Help"].forEach((item) => {
    Button({
      parent: sidebar,
      label: item,
      onClick: () => console.log(`Clicked ${item}`),
    });
  });

  Label({
    parent: content,
    text: "Welcome to AlloyScript",
  });

  TextField({
    parent: content,
    placeholder: "Type something...",
    onChange: (val) => console.log(`Input: ${val}`),
  });

  Button({
    parent: content,
    label: "Submit",
    onClick: async () => {
        const file = Alloy.file("output.txt");
        await Alloy.write(file, "Data submitted from GUI");
        console.log("Data saved to output.txt");
    }
  });
};

app();
