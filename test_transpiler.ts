const transpiler = new Bun.Transpiler({ loader: 'ts' });
const code = 'const x: number = 1;';
console.log(transpiler.transformSync(code));
