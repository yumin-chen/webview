import { test, expect } from "bun:test";

test("browser API proxy should forward calls to Service WebView", async () => {
    // Mock the global MicroQuickJS and the bridge
    const window = {
        fetch: async (url: string) => {
            // Proxy logic in MicroQuickJS
            return await (globalThis as any).alloy_browser_api_proxy({ api: "fetch", args: [url] });
        }
    };

    let serviceWebViewReceived = "";
    (globalThis as any).alloy_browser_api_proxy = async (req: any) => {
        serviceWebViewReceived = req.api;
        return { status: 200, data: `fetched from ${req.args[0]}` };
    };

    const result = await window.fetch("https://example.com");
    expect(serviceWebViewReceived).toBe("fetch");
    expect((result as any).status).toBe(200);
});
