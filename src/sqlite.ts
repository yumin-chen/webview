declare global {
  interface Window {
    Alloy: {
      spawn: (command: string, args: string[]) => Promise<number>;
      spawnSync: (command: string, args: string[]) => number;
      sqlite: {
          open: (filename: string, options: any) => number; // returns db_id
          query: (db_id: number, sql: string) => number; // returns stmt_id
          run: (db_id: number, sql: string, params: any) => { lastInsertRowid: number; changes: number };
          serialize: (db_id: number) => string; // base64
          deserialize: (contents: string) => number; // returns db_id
          loadExtension: (db_id: number, name: string) => void;
          fileControl: (db_id: number, cmd: number, value: any) => void;
          setCustomSQLite: (path: string) => void;
          stmt_all: (stmt_id: number, params: any) => any[];
          stmt_get: (stmt_id: number, params: any) => any;
          stmt_run: (stmt_id: number, params: any) => { lastInsertRowid: number; changes: number };
          stmt_values: (stmt_id: number, params: any) => any[][];
          stmt_finalize: (stmt_id: number) => void;
          stmt_toString: (stmt_id: number) => string;
          stmt_metadata: (stmt_id: number) => { columnNames: string[], columnTypes: string[], declaredTypes: (string|null)[], paramsCount: number };
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

  constructor(db_id: number, stmt_id: number) {
    this._db_id = db_id;
    this._stmt_id = stmt_id;
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

  private _handleConversions(row: any): any {
      if (!row) return row;
      for (const key in row) {
          const val = row[key];
          if (typeof val === "string" && val.endsWith("n") && /^-?\d+n$/.test(val)) {
              row[key] = BigInt(val.slice(0, -1));
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
      for (const param of params) {
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
    const results = window.Alloy.sqlite.stmt_all(this._stmt_id, params).map(r => this._handleConversions(r));
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
    const result = this._handleConversions(window.Alloy.sqlite.stmt_get(this._stmt_id, params));
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
    const stmt = new Statement<T, ParamsType>(this._db_id, this._stmt_id);
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
  private _safeIntegers: boolean = false;

  constructor(filename: string = ":memory:", options: any = {}) {
    if (typeof options === "number") {
        options = { readwrite: !!(options & 2), create: !!(options & 4) };
    }
    this._safeIntegers = options.safeIntegers || false;
    this._db_id = window.Alloy.sqlite.open(filename, options);
  }

  static deserialize(contents: Uint8Array): Database {
      // Binary to base64 for bridge
      const base64 = btoa(String.fromCharCode(...contents));
      const db_id = window.Alloy.sqlite.deserialize(base64);
      const db = new Database(":memory:"); // Dummy open, we override db_id
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
    const stmt = new Statement<ReturnType, ParamsType>(this._db_id, stmt_id);
    this._queryCache.set(sql, stmt);
    return stmt;
  }

  prepare<ReturnType = any, ParamsType = any>(sql: string): Statement<ReturnType, ParamsType> {
    const stmt_id = window.Alloy.sqlite.query(this._db_id, sql);
    return new Statement<ReturnType, ParamsType>(this._db_id, stmt_id);
  }

  run(sql: string, params?: SQLQueryBindings): { lastInsertRowid: number; changes: number } {
    if (params) this.query(sql).run(params);
    return window.Alloy.sqlite.run(this._db_id, sql, params);
  }

  exec(sql: string, params?: SQLQueryBindings): { lastInsertRowid: number; changes: number } {
      return this.run(sql, params);
  }

  transaction(insideTransaction: (...args: any) => any): any {
      const wrapper = (...args: any[]) => {
          this.run("BEGIN TRANSACTION");
          try {
              const result = insideTransaction.apply(this, args);
              this.run("COMMIT");
              return result;
          } catch (e) {
              this.run("ROLLBACK");
              throw e;
          }
      };

      (wrapper as any).deferred = (...args: any[]) => {
          this.run("BEGIN DEFERRED TRANSACTION");
          try {
              const result = insideTransaction.apply(this, args);
              this.run("COMMIT");
              return result;
          } catch (e) {
              this.run("ROLLBACK");
              throw e;
          }
      };

      (wrapper as any).immediate = (...args: any[]) => {
          this.run("BEGIN IMMEDIATE TRANSACTION");
          try {
              const result = insideTransaction.apply(this, args);
              this.run("COMMIT");
              return result;
          } catch (e) {
              this.run("ROLLBACK");
              throw e;
          }
      };

      (wrapper as any).exclusive = (...args: any[]) => {
          this.run("BEGIN EXCLUSIVE TRANSACTION");
          try {
              const result = insideTransaction.apply(this, args);
              this.run("COMMIT");
              return result;
          } catch (e) {
              this.run("ROLLBACK");
              throw e;
          }
      };

      return wrapper;
  }

  close(throwOnError: boolean = false): void {
    window.Alloy.sqlite.close(this._db_id);
  }

  [Symbol.dispose]() {
      this.close();
  }
}

export const constants = {
    SQLITE_FCNTL_PERSIST_WAL: 10 // Example constant
};
