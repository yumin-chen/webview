import { Alloy } from "../../src/alloyscript/runtime";

console.log("Bind Example Running...");

Alloy.Transpiler.bind_global("CustomAPI", {
    hello: () => "world"
});

const result = Alloy.secureEval("CustomAPI.hello()");
console.log("Secure Eval Result:", result);
