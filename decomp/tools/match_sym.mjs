// Matcher with relocation resolution ("linker-lite") for global-referencing functions.
// THPS1 is -G8: small globals are addressed gp-relative (gp = 0x800d1080). Reference a global
// by name in the C (define it, e.g. `unsigned char g;`), provide its address in a symbol map,
// and this resolves psyq-asm's relocations (tools/psyq.mjs link) before byte-diffing vs SLUS_008.60.
// A switch's .rdata jump table is placed at its retail address and checked word-for-word too, and
// string literals at the retail addresses the code names (psyq.mjs link).
//
// Usage: node tools/match_sym.mjs <src.c> <retailAddrHex> <size> <syms.json> [-O2]
//   syms.json: { "gp": "0x800d1080", "<name>": "0x........", [".rdata": "0x........"], ... }
import { readFileSync } from 'node:fs';
import { assemble, decodeWords } from 'psyq-asm';
import { cc1, link, retailWords, alignedScore } from './psyq.mjs';

const G = 8;

const [src, addrHex, sizeStr, symPath, ...flags] = process.argv.slice(2);
const addr = parseInt(addrHex, 16), size = parseInt(sizeStr, 10);
let opt = '-O2';
for (const f of flags) if (f.startsWith('-O')) opt = f;
const syms = JSON.parse(readFileSync(symPath, 'utf8'));

const sfile = `build/sym_${process.pid}.s`;
cc1(src, sfile, opt, G);
const a = assemble(readFileSync(sfile, 'utf8'), { gpSize: G });
const { words, data } = link(a.object, addr, syms);
const want = retailWords(addr, size >> 2);
for (const d of data) console.log(d.skipped ? `${d.section}: not placed in the retail image (compiler-emitted data), not checked` : d.pieces ? `${d.section}: ${d.total} pieces (string literals...) at the retail addresses the code names, bytes match` : `${d.section} @0x${d.base.toString(16)}: ${d.same}/${d.total} words match retail`);
const R = ['zero','at','v0','v1','a0','a1','a2','a3','t0','t1','t2','t3','t4','t5','t6','t7','s0','s1','s2','s3','s4','s5','s6','s7','t8','t9','k0','k1','gp','sp','fp','ra'];
const f = o => o.kind === 'gpr' ? R[o.number] : o.kind === 'mem' ? `${o.offset}(${R[o.base]})` : o.kind === 'imm' ? o.value : (o.target ?? '');
const fm = w => decodeWords(Uint32Array.from(w)).instructions.map(i => `${i.mnemonic} ${(i.operands||[]).map(f).join(',')}`);
const gA = fm(words), wA = fm(want);
let same = 0;
for (let i = 0; i < Math.max(words.length, want.length); i++) {
  const ok = words[i] === want[i]; if (ok) same++;
  if ((!ok && !process.env.D) || process.env.V) console.log(`${String(i).padStart(2)} ${(gA[i]||'').padEnd(20)} | ${(wA[i]||'').padEnd(20)} ${ok?'':'X'}`);
}
// D=1: alignment-aware diff. LCS-align the two word streams and print only the differing hunks, so
// one inserted/missing instruction doesn't drown the real differences.
if (process.env.D && (same !== want.length || words.length !== want.length)) {
  const n = words.length, m = want.length, L = Array.from({ length: n + 1 }, () => new Uint16Array(m + 1));
  for (let i = n - 1; i >= 0; i--) for (let j = m - 1; j >= 0; j--)
    L[i][j] = words[i] === want[j] ? L[i + 1][j + 1] + 1 : Math.max(L[i + 1][j], L[i][j + 1]);
  let i = 0, j = 0, mine = [], ret = [];
  const flush = () => {
    if (!mine.length && !ret.length) return;
    console.log(`@@ mine ${mine[0]?.[0] ?? i} | retail ${ret[0]?.[0] ?? j}`);
    for (let k = 0; k < Math.max(mine.length, ret.length); k++)
      console.log(`  ${(mine[k] ? `${mine[k][0]} ${gA[mine[k][0]]}` : '').padEnd(30)} | ${ret[k] ? `${ret[k][0]} ${wA[ret[k][0]]}` : ''}`);
    mine = []; ret = [];
  };
  while (i < n || j < m) {
    if (i < n && j < m && words[i] === want[j]) { flush(); i++; j++; }
    else if (j >= m || (i < n && L[i + 1][j] >= L[i][j + 1])) mine.push([i++]);
    else ret.push([j++]);
  }
  flush();
}
const dataOk = data.every(d => d.same === d.total);
if (same !== want.length || words.length !== want.length) console.log(`aligned ${alignedScore(words, want)}/${want.length} (${words.length} words compiled)`);
console.log(`${same}/${want.length}${same===want.length&&words.length===want.length&&dataOk?'  >>> EXACT <<<':''}`);
