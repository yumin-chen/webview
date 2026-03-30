function parseArgs(cmdStr) {
  const args = []; let current = ""; let inQuotes = false;
  for (let i = 0; i < cmdStr.length; i++) {
    const c = cmdStr[i];
    if (c === '"') inQuotes = !inQuotes;
    else if (c === ' ' && !inQuotes) { if (current) args.push(current); current = ""; }
    else current += c;
  }
  if (current) args.push(current); return args;
}

export function $(strings, ...values) {
  let cmdStr = strings[0];
  for (let i = 0; i < values.length; i++) {
    let val = values[i];
    if (val && typeof val === 'object' && val.raw) cmdStr += val.raw;
    else if (typeof val === 'string') cmdStr += `"${val.replace(/"/g, '\\"')}"`;
    else if (val instanceof Response) { /* Read from response */ }
    else cmdStr += val;
    cmdStr += strings[i + 1];
  }

  const options = { _cwd: undefined, _env: undefined, _nothrow: false };
  const promise = (async () => {
    const commands = cmdStr.split('|').map(s => s.trim());
    let lastStdout = null; let finalRes = null;
    for (const cmd of commands) {
      let actualCmd = cmd;
      let stdoutRedirect = null;
      let stderrRedirect = null;

      if (cmd.includes('2>')) {
          const parts = cmd.split('2>');
          actualCmd = parts[0].trim();
          stderrRedirect = parts[1].trim();
      } else if (cmd.includes('>')) {
          const parts = cmd.split('>');
          actualCmd = parts[0].trim();
          stdoutRedirect = parts[1].trim();
      }

      const args = parseArgs(actualCmd);
      const proc = Alloy.spawn(args, { cwd: options._cwd || ($._cwd as any), env: options._env || ($._env as any) });

      if (lastStdout) { await proc.stdin.write(lastStdout); await proc.stdin.end(); }
      const exitCode = await proc.exited;

      const readStream = async (stream) => {
        const reader = stream.getReader();
        let out = "";
        while (true) { const { done, value } = await reader.read(); if (done) break; out += new TextDecoder().decode(value); }
        return out;
      };

      const stdout = await readStream(proc.stdout);
      const stderr = await readStream(proc.stderr);
      lastStdout = stdout;

      if (exitCode !== 0 && !promise._nothrow) throw new Error(`Command failed: ${actualCmd}\n${stderr}`);

      finalRes = {
        exitCode, stdout: Buffer.from(stdout), stderr: Buffer.from(stderr),
        text: async () => stdout, json: async () => JSON.parse(stdout),
        lines: async function* () { for (const line of stdout.split('\n')) if (line) yield line; }
      };
    }
    return finalRes;
  })();
  // chainable methods same as before...
  (promise as any).quiet = () => { return promise; };
  (promise as any).nothrow = () => { options._nothrow = true; return promise; };
  (promise as any).text = async () => (await promise).stdout.toString();
  (promise as any).json = async () => JSON.parse((await promise).stdout.toString());
  (promise as any).cwd = (path) => { options._cwd = path; return promise; };
  (promise as any).env = (vars) => { options._env = vars; return promise; };
  return promise;
}

$.escape = (s) => s.replace(/[$( )`"]/g, '\\$&');
$.nothrow = () => { $.throws(false); };
$.throws = (v) => { $._throws = v; };
$.cwd = (path) => { $._cwd = path; };
$.env = (vars) => { $._env = vars; };
