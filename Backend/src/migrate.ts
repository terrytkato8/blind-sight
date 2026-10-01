import { readdirSync, readFileSync } from 'node:fs';
import { join, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';
import { pool } from './db.js';

const here = dirname(fileURLToPath(import.meta.url));
const dir = join(here, '..', 'migrations');

const files = readdirSync(dir).filter((f) => f.endsWith('.sql')).sort();
for (const f of files) {
  process.stdout.write(`applying ${f} ... `);
  await pool.query(readFileSync(join(dir, f), 'utf8'));
  console.log('ok');
}
await pool.end();
