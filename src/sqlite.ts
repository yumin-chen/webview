declare global {
  interface Window {
    Alloy: {
      spawn: (command: string, args: string[]) => Promise<number>;
      spawnSync: (command: string, args: string[]) => number;
      sqlite: {
          open: (filename: string, options: any) => number; // returns db_id
          query: (db_id: number, sql: string) => number; // returns stmt_id
          run: (db_id: number, sql: string, params: any) => { lastInsertRowid: number; changes: number };
          stmt_all: (stmt_id: number, params: any) => any[];
          stmt_get: (stmt_id: number, params: any) => any;
          stmt_run: (stmt_id: number, params: any) => { lastInsertRowid: number; changes: number };
          stmt_values: (stmt_id: number, params: any) => any[][];
          stmt_finalize: (stmt_id: number) => void;
          stmt_toString: (stmt_id: number) => string;
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

  constructor(db_id: number, stmt_id: number) {
    this._db_id = db_id;
    this._stmt_id = stmt_id;
  }

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

  all(...params: ParamsType[]): ReturnType[] {
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
    const result = this._handleConversions(window.Alloy.sqlite.stmt_get(this._stmt_id, params));
    if (result && this._Class) {
        const obj = Object.create(this._Class.prototype);
        Object.assign(obj, result);
        return obj;
    }
    return result;
  }

  run(...params: ParamsType[]): { lastInsertRowid: number; changes: number } {
    return window.Alloy.sqlite.stmt_run(this._stmt_id, params);
  }

  values(...params: ParamsType[]): unknown[][] {
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

  // [Symbol.iterator] for iterate()
  *[Symbol.iterator](): IterableIterator<ReturnType> {
      // Simple implementation using all() for now, though docs suggest incremental
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

  constructor(filename: string = ":memory:", options: any = {}) {
    this._db_id = window.Alloy.sqlite.open(filename, options);
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
    return window.Alloy.sqlite.run(this._db_id, sql, params);
  }

  transaction(insideTransaction: (...args: any) => any): any {
      const wrapper = (...args: any[]) => {
          this.run("BEGIN TRANSACTION");
          try {
              const result = insideTransaction(...args);
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
              const result = insideTransaction(...args);
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
              const result = insideTransaction(...args);
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
              const result = insideTransaction(...args);
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
