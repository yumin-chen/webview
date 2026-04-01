export const Alloy = {
    Transpiler: class {
        constructor(options) {
            this.options = options || {};
        }
        transformSync(code, loader) {
            // Browser fallback or WASM-based transform
            return code;
        }
        async transform(code, loader) {
            return this.transformSync(code, loader);
        }
    }
};

export default { Alloy };
