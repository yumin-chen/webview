import { Database } from "Alloy:sqlite";

class Statement {
  constructor(dbId, sql) {
    this.dbId = dbId;
    this.sql = sql;
    this.id = window.__alloy_sqlite_query(dbId, sql);
    if (this.id.startsWith('{')) throw new Error(JSON.parse(this.id).error);
  }
  get(...params) {
    // TODO: bind params
    const res = window.__alloy_sqlite_step(this.id);
    return res ? JSON.parse(res) : undefined;
  }
  all(...params) {
    const results = [];
    let res;
    while (res = window.__alloy_sqlite_step(this.id)) {
      results.push(JSON.parse(res));
    }
    return results;
  }
  run(...params) {
    window.__alloy_sqlite_step(this.id);
    return { lastInsertRowid: 0, changes: 0 };
  }
  values(...params) {
    const rows = this.all(...params);
    return rows.map(r => Object.values(r));
  }
  finalize() { /* window.__alloy_sqlite_finalize(this.id) */ }
  toString() { return this.sql; }
  as(Cls) {
    this._asClass = Cls;
    return this;
  }
}

class DatabaseImpl {
  constructor(filename, options) {
    this.id = window.__alloy_sqlite_open(filename || ":memory:");
    if (this.id.startsWith('{')) throw new Error(JSON.parse(this.id).error);
  }
  query(sql) {
    return new Statement(this.id, sql);
  }
  prepare(sql) {
    return new Statement(this.id, sql);
  }
  run(sql, params) {
    return this.query(sql).run(params);
  }
  transaction(fn) {
    const wrapper = (...args) => {
      this.run("BEGIN");
      try {
        const res = fn(...args);
        this.run("COMMIT");
        return res;
      } catch (e) {
        this.run("ROLLBACK");
        throw e;
      }
    };
    return wrapper;
  }
  close() {}
}

export { DatabaseImpl as Database };
