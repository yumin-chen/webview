/**
 * AlloyScript Runtime
 * Under CC0 Unlicense Public Domain
 */

export namespace Alloy {
    export interface SpawnOptions {
        cwd?: string;
        env?: Record<string, string>;
        stdio?: "inherit" | "pipe" | "ignore";
    }

    export interface SpawnResult {
        stdout: Uint8Array;
        stderr: Uint8Array;
        status: number;
    }

    /**
     * Spawns a process synchronously.
     */
    export function spawnSync(command: string, args: string[], options?: SpawnOptions): SpawnResult {
        // @ts-ignore
        return AlloyInternal.spawnSync(command, args, options);
    }

    /**
     * Spawns a process asynchronously.
     */
    export async function spawn(command: string, args: string[], options?: SpawnOptions): Promise<SpawnResult> {
        // @ts-ignore
        return AlloyInternal.spawn(command, args, options);
    }

    /**
     * Evaluates code in a secure MicroQuickJS context.
     */
    export function secureEval(code: string): any {
        // @ts-ignore
        return AlloyInternal.secureEval(code);
    }

    export type Loader = "jsx" | "js" | "ts" | "tsx";

    export interface TranspilerOptions {
        define?: Record<string, string>;
        loader?: Loader;
        target?: "browser" | "Alloy" | "node";
        tsconfig?: string;
        minifyWhitespace?: boolean;
        inline?: boolean;
    }

    export class Transpiler {
        private options: TranspilerOptions;

        constructor(options: TranspilerOptions) {
            this.options = options;
        }

        /**
         * Binds a global object or function to the engine.
         */
        static bind_global(name: string, value: any): void {
            // @ts-ignore
            AlloyInternal.bindGlobal(name, value);
        }

        transformSync(code: string, loader?: Loader): string {
            // @ts-ignore
            return AlloyInternal.transpilerTransformSync(code, loader || this.options.loader, this.options);
        }

        async transform(code: string, loader?: Loader): Promise<string> {
            // @ts-ignore
            return AlloyInternal.transpilerTransform(code, loader || this.options.loader, this.options);
        }

        scan(code: string): { exports: string[], imports: any[] } {
            // @ts-ignore
            return AlloyInternal.transpilerScan(code, this.options);
        }

        scanImports(code: string): any[] {
            // @ts-ignore
            return AlloyInternal.transpilerScanImports(code, this.options);
        }
    }
}

// Global Alloy binding
if (typeof globalThis !== 'undefined') {
    (globalThis as any).Alloy = Alloy;
}
