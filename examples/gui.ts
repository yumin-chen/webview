const { gui } = Alloy.import("Alloy:gui");

async function main() {
    const win = new gui.Window("Alloy Native GUI", 600, 400);
    await win.init();

    const vs = new gui.VStack(win);
    await vs.init();

    const lbl = new gui.Label(vs);
    await lbl.init();
    lbl.setText("Alloy Native Dashboard");

    const hs = new gui.HStack(vs);
    await hs.init();

    const btn = new gui.Button(hs);
    await btn.init();
    btn.setText("Click Me");

    const pb = new gui.ProgressBar(vs);
    await pb.init();
    pb.setValue(0.65);

    // Detect OS for specific styling or behavior if needed
    const isMacOS = navigator.platform.toUpperCase().indexOf('MAC') >= 0;
    if (isMacOS) {
        lbl.setText(lbl.getText() + " (macOS)");
    }

    win.run();
}

main().catch(console.error);
