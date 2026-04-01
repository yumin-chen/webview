import { test, expect } from "bun:test";

test("Alloy.build should return compiled bytecode", async () => {
    // Mock the global Alloy and bridge
    const Alloy = {
        build: (source: string) => {
            return (globalThis as any).alloy_build(source);
        }
    };

    (globalThis as any).alloy_build = (source: string) => {
        return `compiled:${source}`;
    };

    const result = Alloy.build("const x = 1;");
    expect(result).toBe("compiled:const x = 1;");
});

test("Alloy.browserApiProxy should delegate to service bridge", async () => {
    let bridgeReceived = "";
    (globalThis as any).alloy_browser_api_proxy = (req: any) => {
        bridgeReceived = req.api;
        return { ok: true };
    };

    const windowProxy = {
        fetch: (url: string) => (globalThis as any).alloy_browser_api_proxy({ api: "fetch", args: [url] })
    };

    const res = windowProxy.fetch("https://test.com");
    expect(bridgeReceived).toBe("fetch");
    expect((res as any).ok).toBe(true);
});
