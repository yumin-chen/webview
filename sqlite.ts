class Statement {
  constructor(dbId, sql) {
    this.dbId = dbId;
    this.sql = sql;
    this.id = window.__alloy_sqlite_query(dbId, sql);
    if (this.id.startsWith('{')) throw new Error(JSON.parse(this.id).error);
    this.columnNames = [];
    this.columnTypes = [];
    this.paramsCount = 0;
  }

  _bind(params) {
    window.__alloy_sqlite_reset(this.id);
    if (params.length > 0) {
      params.forEach((p, i) => {
        window.__alloy_sqlite_bind(this.id, i + 1, p === null ? "null" : p.toString());
      });
    }
  }

  get(...params) {
    this._bind(params);
    const res = window.__alloy_sqlite_step(this.id);
    return res ? JSON.parse(res) : undefined;
  }

  all(...params) {
    this._bind(params);
    const results = [];
    let res;
    while (res = window.__alloy_sqlite_step(this.id)) {
      results.push(JSON.parse(res));
    }
    return results;
  }

  run(...params) {
    this._bind(params);
    window.__alloy_sqlite_step(this.id);
    return { lastInsertRowid: 0, changes: 0 };
  }

  values(...params) {
    const rows = this.all(...params);
    return rows.map(r => Object.values(r));
  }

  finalize() {
    window.__alloy_sqlite_reset(this.id);
  }

  toString() {
    return this.sql;
  }

  as(Cls) {
    this._asClass = Cls;
    return this;
  }
}

class Database {
  constructor(filename, options = {}) {
    this.id = window.__alloy_sqlite_open(filename || ":memory:");
    if (this.id.startsWith('{')) throw new Error(JSON.parse(this.id).error);
  }

  query(sql) {
    return new Statement(this.id, sql);
  }

  prepare(sql) {
    return new Statement(this.id, sql);
  }

  run(sql, params = []) {
    return this.query(sql).run(...(Array.isArray(params) ? params : [params]));
  }

  serialize() {
    const hex = window.__alloy_sqlite_serialize(this.id);
    const bytes = new Uint8Array(hex.length / 2);
    for (let i = 0; i < hex.length; i += 2) {
      bytes[i / 2] = parseInt(hex.substring(i, i + 2), 16);
    }
    return bytes;
  }

  transaction(fn) {
    const t = (args) => {
      this.run("BEGIN");
      try {
        const res = fn(args);
        this.run("COMMIT");
        return res;
      } catch (e) {
        this.run("ROLLBACK");
        throw e;
      }
    };
    t.deferred = (args) => { this.run("BEGIN DEFERRED"); try { const res = fn(args); this.run("COMMIT"); return res; } catch(e) { this.run("ROLLBACK"); throw e; } };
    t.immediate = (args) => { this.run("BEGIN IMMEDIATE"); try { const res = fn(args); this.run("COMMIT"); return res; } catch(e) { this.run("ROLLBACK"); throw e; } };
    t.exclusive = (args) => { this.run("BEGIN EXCLUSIVE"); try { const res = fn(args); this.run("COMMIT"); return res; } catch(e) { this.run("ROLLBACK"); throw e; } };
    return t;
  }

  close() {}
}

export { Database };
