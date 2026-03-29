export function $(strings, ...values) {
  let cmdStr = strings[0];
  for (let i = 0; i < values.length; i++) {
    let val = values[i];
    if (val && typeof val === 'object' && val.raw) {
      cmdStr += val.raw;
    } else if (typeof val === 'string') {
      cmdStr += `"${val.replace(/"/g, '\\"')}"`; // Basic escaping
    } else {
      cmdStr += val;
    }
    cmdStr += strings[i + 1];
  }

  const args = cmdStr.trim().split(/\s+/);

  const promise = (async () => {
    const proc = Alloy.spawn(args);
    const exitCode = await proc.exited;

    const reader = proc.stdout.getReader();
    let stdout = "";
    const decoder = new TextDecoder();
    while (true) {
      const { done, value } = await reader.read();
      if (done) break;
      stdout += decoder.decode(value);
    }

    if (exitCode !== 0 && !promise._nothrow) {
      throw new Error(`Command failed with code ${exitCode}`);
    }

    return {
      exitCode,
      stdout: Buffer.from(stdout),
      stderr: Buffer.from(""),
      text: async () => stdout,
      json: async () => JSON.parse(stdout),
      blob: async () => new Blob([stdout], { type: "text/plain" }),
      lines: async function* () {
        const lines = stdout.split('\n');
        for (const line of lines) {
          if (line) yield line;
        }
      }
    };
  })();

  promise.quiet = () => { return promise; };
  promise.nothrow = () => { promise._nothrow = true; return promise; };
  promise.text = async () => {
    const res = await promise;
    return res.stdout.toString();
  };
  promise.json = async () => {
    const res = await promise;
    return JSON.parse(res.stdout.toString());
  };

  return promise;
}

$.escape = (s) => s.replace(/[$( )`"]/g, '\\$&');
$.nothrow = () => { $.throws(false); };
$.throws = (v) => { /* Global setting logic */ };
