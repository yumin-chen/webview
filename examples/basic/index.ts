import { Alloy } from "../../src/alloyscript/runtime";

console.log("Basic Example Running...");

async function run() {
    const result = await Alloy.spawn("ls", ["-la"]);
    console.log("Spawn Result Status:", result.status);
}

run().catch(console.error);
