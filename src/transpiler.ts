export type Loader = "jsx" | "js" | "ts" | "tsx";

export interface TranspilerOptions {
  define?: Record<string, string>;
  loader?: Loader;
  target?: "browser" | "Alloy" | "node" | "AlloyScript";
  tsconfig?: string | any;
  macro?: Record<string, Record<string, string>>;
  exports?: {
    eliminate?: string[];
    replace?: Record<string, string>;
  };
  trimUnusedImports?: boolean;
  minifyWhitespace?: boolean;
  inline?: boolean;
}

export type ImportKind =
  | "import-statement"
  | "require-call"
  | "require-resolve"
  | "dynamic-import"
  | "import-rule"
  | "url-token"
  | "internal"
  | "entry-point-build"
  | "entry-point-run";

export interface Import {
  path: string;
  kind: ImportKind;
}

declare global {
  interface Window {
    Alloy: {
      transpiler_transform: (code: string, loader?: string, options?: string) => Promise<string>;
      transpiler_transform_sync: (code: string, loader?: string, options?: string) => string;
      transpiler_scan: (code: string) => string; // returns JSON
    };
  }
}

export class Transpiler {
  private options: TranspilerOptions;

  constructor(options: TranspilerOptions = {}) {
    this.options = options;
  }

  async transform(code: string, loader?: Loader): Promise<string> {
    const l = loader || this.options.loader;
    return window.Alloy.transpiler_transform(code, l, JSON.stringify(this.options));
  }

  transformSync(code: string, loader?: Loader): string {
    const l = loader || this.options.loader;
    return window.Alloy.transpiler_transform_sync(code, l, JSON.stringify(this.options));
  }

  scan(code: string): { exports: string[]; imports: Import[] } {
    const result = window.Alloy.transpiler_scan(code);
    return JSON.parse(result);
  }

  scanImports(code: string): Import[] {
    const result = this.scan(code);
    return result.imports;
  }
}
