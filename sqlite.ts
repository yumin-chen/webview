class Statement {
  constructor(dbId, sql, dbOptions) {
    this.dbId = dbId;
    this.sql = sql;
    this.dbOptions = dbOptions;
    this.id = window.__alloy_sqlite_query(dbId, sql);
    if (this.id.startsWith('{')) throw new Error(JSON.parse(this.id).error);
  }

  _process(row) {
    if (!row) return undefined;
    const obj = JSON.parse(row);
    for (const key in obj) {
      if (obj[key] && typeof obj[key] === 'object' && obj[key].__blob) {
        const hex = obj[key].__blob;
        const bytes = new Uint8Array(hex.length / 2);
        for (let i = 0; i < hex.length; i += 2) bytes[i / 2] = parseInt(hex.substring(i, i + 2), 16);
        obj[key] = bytes;
      } else if (typeof obj[key] === 'string' && obj[key].endsWith('n')) {
        obj[key] = BigInt(obj[key].slice(0, -1));
      }
    }
    if (this._asClass) {
      const instance = Object.create(this._asClass.prototype);
      Object.assign(instance, obj);
      return instance;
    }
    return obj;
  }

  _bind(params) {
    window.__alloy_sqlite_reset(this.id);
    params.forEach((p, i) => {
      window.__alloy_sqlite_bind(this.id, i + 1, p === null ? "null" : p.toString());
    });
  }

  get(...params) {
    this._bind(params);
    return this._process(window.__alloy_sqlite_step(this.id, !!this.dbOptions.safeIntegers));
  }

  all(...params) {
    this._bind(params);
    const results = [];
    let res;
    while (res = window.__alloy_sqlite_step(this.id, !!this.dbOptions.safeIntegers)) {
      results.push(this._process(res));
    }
    return results;
  }

  run(...params) {
    this._bind(params);
    window.__alloy_sqlite_step(this.id, !!this.dbOptions.safeIntegers);
    return { lastInsertRowid: 0, changes: 0 };
  }

  as(Cls) { this._asClass = Cls; return this; }
}

class Database {
  constructor(filename, options = {}) {
    this.options = options;
    this.id = window.__alloy_sqlite_open(filename || ":memory:");
    if (this.id.startsWith('{')) throw new Error(JSON.parse(this.id).error);
  }
  query(sql) { return new Statement(this.id, sql, this.options); }
  prepare(sql) { return this.query(sql); }
  run(sql, params = []) { return this.query(sql).run(...(Array.isArray(params) ? params : [params])); }
  transaction(fn) {
    const t = (args) => {
      this.run("BEGIN");
      try { const res = fn(args); this.run("COMMIT"); return res; }
      catch (e) { this.run("ROLLBACK"); throw e; }
    };
    return t;
  }
}

export { Database };
