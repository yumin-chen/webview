declare global {
  interface Window {
    Alloy: {
      spawn: (command: string, args: string[]) => Promise<number>;
      spawnSync: (command: string, args: string[]) => number;
      secureEval: (code: string) => string;
      sqlite: {
          open: (filename: string, options: any) => number;
          query: (db_id: number, sql: string) => number;
          run: (db_id: number, sql: string, params: any) => { lastInsertRowid: number; changes: number };
          serialize: (db_id: number) => string;
          deserialize: (contents: string) => number;
          loadExtension: (db_id: number, name: string) => void;
          fileControl: (db_id: number, cmd: number, value: any) => void;
          setCustomSQLite: (path: string) => void;
          stmt_all: (stmt_id: number, params: any) => any[];
          stmt_get: (stmt_id: number, params: any) => any;
          stmt_run: (stmt_id: number, params: any) => { lastInsertRowid: number; changes: number };
          stmt_values: (stmt_id: number, params: any) => any[][];
          stmt_finalize: (stmt_id: number) => void;
          stmt_toString: (stmt_id: number) => string;
          stmt_metadata: (stmt_id: number) => any;
          close: (db_id: number) => void;
      };
    };
  }
}

export type SQLQueryBindings =
  | string
  | bigint
  | Uint8Array
  | number
  | boolean
  | null
  | Record<string, string | bigint | Uint8Array | number | boolean | null>;

export class Statement<ReturnType = any, ParamsType = any> {
  private _db_id: number;
  private _stmt_id: number;
  private _Class: (new (...args: any[]) => ReturnType) | null = null;
  private _metadata: any = null;
  private _strict: boolean = false;
  private _safeIntegers: boolean = false;

  constructor(db_id: number, stmt_id: number, strict: boolean = false, safeIntegers: boolean = false) {
    this._db_id = db_id;
    this._stmt_id = stmt_id;
    this._strict = strict;
    this._safeIntegers = safeIntegers;
  }

  private _ensureMetadata() {
      if (!this._metadata) {
          this._metadata = window.Alloy.sqlite.stmt_metadata(this._stmt_id);
      }
  }

  get columnNames(): string[] { this._ensureMetadata(); return this._metadata.columnNames; }
  get columnTypes(): string[] { this._ensureMetadata(); return this._metadata.columnTypes; }
  get declaredTypes(): (string | null)[] { this._ensureMetadata(); return this._metadata.declaredTypes; }
  get paramsCount(): number { this._ensureMetadata(); return this._metadata.paramsCount; }
  get native(): any { return { stmt_id: this._stmt_id }; }

  private _handleConversions(row: any, safeIntegers: boolean): any {
      if (!row) return row;
      for (const key in row) {
          const val = row[key];
          if (typeof val === "string" && val.endsWith("n") && /^-?\d+n$/.test(val)) {
              const big = BigInt(val.slice(0, -1));
              if (safeIntegers) {
                  row[key] = big;
              } else {
                  row[key] = Number(big);
              }
          } else if (typeof val === "string" && val.startsWith("blob:")) {
              const base64 = val.slice(5);
              const binaryString = atob(base64);
              const bytes = new Uint8Array(binaryString.length);
              for (let i = 0; i < binaryString.length; i++) {
                  bytes[i] = binaryString.charCodeAt(i);
              }
              row[key] = bytes;
          }
      }
      return row;
  }

  private _validateParams(params: any[]) {
      for (let i = 0; i < params.length; i++) {
          const param = params[i];
          if (typeof param === "bigint") {
              if (param > 9223372036854775807n || param < -9223372036854775808n) {
                  throw new RangeError(`BigInt value '${param}' is out of range`);
              }
          } else if (param && typeof param === "object" && !(param instanceof Uint8Array)) {
              for (const key in param) {
                  const val = (param as any)[key];
                  if (typeof val === "bigint") {
                      if (val > 9223372036854775807n || val < -9223372036854775808n) {
                          throw new RangeError(`BigInt value '${val}' is out of range`);
                      }
                  }
              }
          }
      }
  }

  all(...params: ParamsType[]): ReturnType[] {
    this._validateParams(params);
    const results = window.Alloy.sqlite.stmt_all(this._stmt_id, params).map(r => this._handleConversions(r, (this as any)._safeIntegers));
    if (this._Class) {
        return results.map(r => {
            const obj = Object.create(this._Class!.prototype);
            Object.assign(obj, r);
            return obj;
        });
    }
    return results;
  }

  get(...params: ParamsType[]): ReturnType | null {
    this._validateParams(params);
    const result = this._handleConversions(window.Alloy.sqlite.stmt_get(this._stmt_id, params), (this as any)._safeIntegers);
    if (result && this._Class) {
        const obj = Object.create(this._Class.prototype);
        Object.assign(obj, result);
        return obj;
    }
    return result;
  }

  run(...params: ParamsType[]): { lastInsertRowid: number; changes: number } {
    this._validateParams(params);
    return window.Alloy.sqlite.stmt_run(this._stmt_id, params);
  }

  values(...params: ParamsType[]): unknown[][] {
    this._validateParams(params);
    return window.Alloy.sqlite.stmt_values(this._stmt_id, params);
  }

  finalize(): void {
    window.Alloy.sqlite.stmt_finalize(this._stmt_id);
  }

  toString(): string {
    return window.Alloy.sqlite.stmt_toString(this._stmt_id);
  }

  as<T>(Class: new (...args: any[]) => T): Statement<T, ParamsType> {
    const stmt = new Statement<T, ParamsType>(this._db_id, this._stmt_id, this._strict, this._safeIntegers);
    stmt._Class = Class;
    return stmt;
  }

  *[Symbol.iterator](): IterableIterator<ReturnType> {
      const results = this.all();
      for (const res of results) {
          yield res;
      }
  }

  iterate(): IterableIterator<ReturnType> {
      return this[Symbol.iterator]();
  }
}

export class Database {
  private _db_id: number;
  private _queryCache: Map<string, Statement> = new Map();
  private _strict: boolean = false;
  private _safeIntegers: boolean = false;

  constructor(filename: string = ":memory:", options: any = {}) {
    if (typeof options === "number") {
        options = { readwrite: !!(options & 2), create: !!(options & 4) };
    }
    this._strict = options.strict || false;
    this._safeIntegers = options.safeIntegers || false;
    this._db_id = window.Alloy.sqlite.open(filename, options);
  }

  static deserialize(contents: Uint8Array): Database {
      const base64 = btoa(String.fromCharCode(...contents));
      const db_id = window.Alloy.sqlite.deserialize(base64);
      const db = new Database(":memory:");
      db._db_id = db_id;
      return db;
  }

  static setCustomSQLite(path: string): void {
      window.Alloy.sqlite.setCustomSQLite(path);
  }

  serialize(): Uint8Array {
      const base64 = window.Alloy.sqlite.serialize(this._db_id);
      const binaryString = atob(base64);
      const bytes = new Uint8Array(binaryString.length);
      for (let i = 0; i < binaryString.length; i++) {
          bytes[i] = binaryString.charCodeAt(i);
      }
      return bytes;
  }

  loadExtension(name: string): void {
      window.Alloy.sqlite.loadExtension(this._db_id, name);
  }

  fileControl(cmd: number, value: any): void {
      window.Alloy.sqlite.fileControl(this._db_id, cmd, value);
  }

  query<ReturnType = any, ParamsType = any>(sql: string): Statement<ReturnType, ParamsType> {
    if (this._queryCache.has(sql)) {
        return this._queryCache.get(sql) as Statement<ReturnType, ParamsType>;
    }
    const stmt_id = window.Alloy.sqlite.query(this._db_id, sql);
    const stmt = new Statement<ReturnType, ParamsType>(this._db_id, stmt_id, this._strict, this._safeIntegers);
    this._queryCache.set(sql, stmt);
    return stmt;
  }

  prepare<ReturnType = any, ParamsType = any>(sql: string): Statement<ReturnType, ParamsType> {
    const stmt_id = window.Alloy.sqlite.query(this._db_id, sql);
    return new Statement<ReturnType, ParamsType>(this._db_id, stmt_id, this._strict, this._safeIntegers);
  }

  run(sql: string, params?: SQLQueryBindings): { lastInsertRowid: number; changes: number } {
    return window.Alloy.sqlite.run(this._db_id, sql, params);
  }

  exec(sql: string, params?: SQLQueryBindings): { lastInsertRowid: number; changes: number } {
      return this.run(sql, params);
  }

  transaction(insideTransaction: (...args: any) => any): any {
      const db = this;
      const fn = (...args: any[]) => {
          db.run("BEGIN");
          try {
              const result = insideTransaction.apply(db, args);
              db.run("COMMIT");
              return result;
          } catch (e) {
              db.run("ROLLBACK");
              throw e;
          }
      };
      fn.deferred = (...args: any[]) => {
          db.run("BEGIN DEFERRED");
          try { const result = insideTransaction.apply(db, args); db.run("COMMIT"); return result; }
          catch (e) { db.run("ROLLBACK"); throw e; }
      };
      fn.immediate = (...args: any[]) => {
          db.run("BEGIN IMMEDIATE");
          try { const result = insideTransaction.apply(db, args); db.run("COMMIT"); return result; }
          catch (e) { db.run("ROLLBACK"); throw e; }
      };
      fn.exclusive = (...args: any[]) => {
          db.run("BEGIN EXCLUSIVE");
          try { const result = insideTransaction.apply(db, args); db.run("COMMIT"); return result; }
          catch (e) { db.run("ROLLBACK"); throw e; }
      };
      return fn;
  }

  close(throwOnError: boolean = false): void {
    window.Alloy.sqlite.close(this._db_id);
  }

  [Symbol.dispose]() {
      this.close();
  }
}

export const constants = {
    SQLITE_FCNTL_PERSIST_WAL: 10
};
