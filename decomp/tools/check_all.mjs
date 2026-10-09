// Re-verify every function listed in matched.tsv with its matcher; exits non-zero on any regression.
// Also the reimpl.tsv rows verified by a matcher (port-only C that is byte-exact too), reported
// apart so they don't count twice in the totals.
// Usage: node tools/check_all.mjs [filterSubstring]
import { readFileSync, existsSync } from 'node:fs';
import { execFile } from 'node:child_process';
import { promisify } from 'node:util';
import { cpus } from 'node:os';

const run = promisify(execFile);
const filter = process.argv[2] ?? '';
const tsv = path => readFileSync(path, 'utf8').split('\n')
  .filter(l => l.trim() && !l.startsWith('#')).map(l => l.split('\t'))
  .filter(r => r.join(' ').includes(filter));
const rows = tsv('matched.tsv');
const reimpl = tsv('reimpl.tsv').filter(r => ['cc', 'sym', 'div'].includes(r[3]));

async function check([src, addr, size, tool, name]) {
  const syms = src.replace(/\.cpp$|\.c$|\.s$/, '.syms.json');
  // extra cc1 flags from a `flags=-fa,-fb` note in the header line (a TU built with other options)
  const header = readFileSync(src, 'utf8').split('\n')[0];
  const extra = [...(header.match(/flags=(\S+)/)?.[1] ?? '').split(','),
    ...(process.env.GLOBAL_FLAGS ?? '').split(' ')].filter(Boolean);
  // `psyq=4.6` in the header: reproduced with another installed Psy-Q cc1 (see README); the old
  // cc1s need TMPDIR set to a Windows path
  const ver = header.match(/psyq=([\d.]+)/)?.[1];
  const ccEnv = ver ? { CC1: `D:/Tools/psyq${ver}/CC1PSX.EXE`, CC1PL: `D:/Tools/psyq${ver}/CC1PLPSX.EXE`,
    TMPDIR: process.cwd().replace(/\//g, '\\') + '\\build' } : {};
  const cmd = {
    cc: ['node', ['tools/matchcc.mjs', src, addr, size, '-O2', '-G0']],
    sym: ['node', ['tools/match_sym.mjs', src, addr, size, syms, ['-O2', ...extra].join(' ')],
      { env: { ...process.env, ...ccEnv } }],
    div: ['python', ['tools/match_div.py', src, addr, size, ...(existsSync(syms) ? [syms] : []), '-O2', '-G8'],
      { env: { ...process.env, ...ccEnv, CC1_OPTS: extra.join(' ') } }],
  }[tool];
  let out;
  try { out = (await run(...cmd)).stdout; } catch (e) { out = (e.stdout || '') + (e.stderr || e.message); }
  const last = out.trim().split('\n').pop();
  return { name, src, size: +size, ok: last.includes('EXACT'), last };
}

const results = [], extra = [];
const queue = [...rows.map(r => [r, results]), ...reimpl.map(r => [r, extra])];
await Promise.all(Array.from({ length: Math.max(1, cpus().length - 1) }, async () => {
  while (queue.length) { const [r, into] = queue.shift(); into.push(await check(r)); }
}));
results.sort((a, b) => a.src.localeCompare(b.src));
for (const r of results) console.log(`${r.ok ? 'ok  ' : 'FAIL'} ${r.src.padEnd(34)} ${r.last}`);
const bad = results.filter(r => !r.ok).length;
extra.sort((a, b) => a.src.localeCompare(b.src));
for (const r of extra) console.log(`${r.ok ? 'ok  ' : 'FAIL'} ${r.src.padEnd(34)} ${r.last}  (reimpl.tsv)`);
const badExtra = extra.filter(r => !r.ok).length;
// game code = .text up to the end of M3d_PreprocessPulsingColours; the Psy-Q libraries follow it
const GAME_BYTES = 0x800858ac - 0x80010000;
const bytes = results.reduce((n, r) => n + (r.ok ? r.size : 0), 0);
console.log(`\n${results.length - bad}/${results.length} exact, ${bytes} of ${GAME_BYTES} game-code bytes (${(100 * bytes / GAME_BYTES).toFixed(1)}%)`);
if (extra.length) console.log(`${extra.length - badExtra}/${extra.length} reimpl.tsv rows exact`);
process.exit(bad || badExtra ? 1 : 0);
