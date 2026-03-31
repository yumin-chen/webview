export function parseEnv(content: string, existingEnv: Record<string, string> = {}): Record<string, string> {
  const result: Record<string, string> = { ...existingEnv };
  const lines = content.split(/\r?\n/);

  for (let line of lines) {
    line = line.trim();
    if (!line || line.startsWith("#")) continue;

    const match = line.match(/^([^=]+)=(.*)$/);
    if (!match) continue;

    const key = match[1].trim();
    let value = match[2].trim();

    // Handle quotes
    if ((value.startsWith('"') && value.endsWith('"')) ||
        (value.startsWith("'") && value.endsWith("'")) ||
        (value.startsWith("`") && value.endsWith("`"))) {
      value = value.substring(1, value.length - 1);
    }

    // Expansion logic
    let expandedValue = "";
    for (let i = 0; i < value.length; i++) {
      if (value[i] === "$" && (i === 0 || value[i - 1] !== "\\")) {
        let varName = "";
        let j = i + 1;
        while (j < value.length && /[a-zA-Z0-9_]/.test(value[j])) {
          varName += value[j];
          j++;
        }
        if (varName) {
          expandedValue += result[varName] || process.env[varName] || "";
          i = j - 1;
        } else {
          expandedValue += "$";
        }
      } else if (value[i] === "$" && i > 0 && value[i - 1] === "\\") {
        expandedValue = expandedValue.substring(0, expandedValue.length - 1) + "$";
      } else {
        expandedValue += value[i];
      }
    }

    result[key] = expandedValue;
  }
  return result;
}

export function loadEnv(Alloy: any) {
  const nodeEnv = Alloy.env?.NODE_ENV || "development";
  const files = [".env", `.env.${nodeEnv}`, ".env.local"];

  let envVars = { ...Alloy.env };

  for (const file of files) {
    const f = Alloy.file(file);
    if (window.__alloy_file_exists(file) === "true") {
        const content = window.__alloy_file_read(file);
        envVars = parseEnv(content, envVars);
    }
  }

  return envVars;
}
