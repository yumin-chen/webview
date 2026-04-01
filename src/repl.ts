import { Alloy } from "./index";

export class REPL {
  private engine: string = "dual"; // microquickjs + webview

  constructor() {}

  async start() {
    console.log("Welcome to AlloyScript REPL (Dual Engine)");
    console.log("Type .exit to quit");

    while (true) {
      const line = await this.readInput();
      if (line === ".exit") break;

      try {
        const result = await this.evaluate(line);
        console.log(result);
      } catch (e: any) {
        console.error(`Error: ${e.message}`);
      }
    }
  }

  private async readInput(): Promise<string> {
    // In a real REPL, this would read line by line from stdin
    return ""; // Stub
  }

  private async evaluate(code: string): Promise<string> {
    // Uses Alloy.secureEval which delegates to both engines
    return Alloy.secureEval(code);
  }
}
