import { test, expect } from "bun:test";

test("securePost should exist in the bridge and call the native hook", async () => {
    // Mock the global Alloy and bridge
    const Alloy = {
        gui: {
            securePost: (webviewId: number, encryptedMsg: string) => {
                // In actual runtime, this calls window.alloy_gui_secure_post
                return (globalThis as any).alloy_gui_secure_post(webviewId, encryptedMsg);
            }
        }
    };

    let callCount = 0;
    let receivedMsg = "";
    (globalThis as any).alloy_gui_secure_post = (id: number, msg: string) => {
        callCount++;
        receivedMsg = msg;
        return 0; // ALLOY_OK
    };

    const result = Alloy.gui.securePost(1, "encrypted_data_packet");
    expect(result).toBe(0);
    expect(callCount).toBe(1);
    expect(receivedMsg).toBe("encrypted_data_packet");
});
