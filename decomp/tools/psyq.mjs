// Shared compile helpers for the Node matchers: optional CPPPSX preprocessing (for sources that
// #include the Psy-Q SDK headers, e.g. libgte.h / inline_c.h) and the DMPSX GTE-command fixup.
import { readFileSync, writeFileSync, mkdirSync } from 'node:fs';
import { execFileSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';

export const CC1 = process.env.CC1 || 'D:/Tools/psyq4.5/CC1PSX.EXE';
// .cpp sources (Neversoft's code is C++) go through the C++ front end
const CC1PL = process.env.CC1PL || 'D:/Tools/psyq4.5/CC1PLPSX.EXE';
const CPP = process.env.CPP || 'D:/Tools/psyq4.5/CPPPSX.EXE';
const INC = process.env.PSYQ_INC || 'D:/Tools/psyq4.5/include';
// the repo's own headers (include/names.h: readable aliases for g++-mangled names)
const LOCAL_INC = fileURLToPath(new URL('../include', import.meta.url));
// what CCPSX hands cpp for C on the PSX target
const CPP_DEFS = ['-undef', '-D__GNUC__=2', '-D__OPTIMIZE__', '-Dmips', '-D__mips__', '-D__psx__', '-D_LANGUAGE_C'];

// cc1 has no preprocessor (not even comment stripping), so always run CPPPSX first, as CCPSX does.
export function cc1(cfile, sfile, opt = '-O2', g = 0) {
  // Hand-written assembly modules (.s) skip the compiler and go straight to the assembler.
  if (cfile.endsWith('.s')) { mkdirSync('build', { recursive: true }); writeFileSync(sfile, readFileSync(cfile)); return; }
  const input = sfile + '.i';
  mkdirSync('build', { recursive: true });
  execFileSync(CPP, [...CPP_DEFS, '-I', LOCAL_INC, '-I', INC, cfile, input], { stdio: 'ignore' });
  // C++: no exception tables (psyq-asm can't assemble their label-difference words; .text is unaffected)
  const cpp = cfile.endsWith('.cpp');
  execFileSync(cpp && (process.env.CC1PL || !process.env.CC1) ? CC1PL : CC1, [input, '-quiet', ...opt.split(' '), ...(cpp && !process.env.NOEXC ? ['-fno-exceptions'] : []), `-G${g}`, '-o', sfile], { stdio: 'ignore' });
}

// INLINE_C.H emits GTE commands as `.word (n<<6)|0x3f` placeholders that Sony's DMPSX.EXE patches
// into real cop2 words post-assembly; apply the same table (tools/dmpsx_table.json, from
// gen_dmpsx_table.py) to the assembled words.
const DMPSX = new Map(Object.entries(JSON.parse(readFileSync(new URL('./dmpsx_table.json', import.meta.url), 'utf8')))
  .map(([k, v]) => [parseInt(k, 16), parseInt(v, 16) >>> 0]));
export const dmpsx = words => words.map(w => DMPSX.get(w) ?? w);

// Retail image access: the PSX-EXE loads contiguously from TEXT_ADDR, so any code/data address maps
// straight to a file offset.
export const RETAIL = "G:/Decompilation/Tony Hawk's Pro Skater/extern/thps1_retail/SLUS_008.60";
const TEXT_ADDR = 0x80010000, HDR = 0x800;
let retailBuf;
export function retailWords(address, n) {
  retailBuf ??= readFileSync(RETAIL);
  const off = address - TEXT_ADDR + HDR, out = [];
  for (let i = 0; i < n; i++) out.push(retailBuf.readUInt32LE(off + 4 * i));
  return out;
}

// "Linker-lite": place .text at the target address, resolve relocations against syms.json, and
// place other sections (a switch's .rdata jump table, string literals) at their retail address.
// That comes from syms.json (".rdata": "0x…") or is inferred from retail's own HI16/LO16 pair that
// addresses it. The placed data sections are then byte-checked against retail too (jump-table
// entries are .text labels, so this verifies the table, not just the code that indexes it).
//
// A data section the code references at several places can also be placed piece by piece: each
// reference resolves to the address retail's own instruction encodes there, if retail has the
// same bytes at that address (each piece, from its reference to the next one, checked on its
// own). A one-function source's string literals then match wherever retail's whole-file TU put
// them, among other functions' literals, which no single section base reproduces.
// Returns { words, data: [{ section, base, same, total }] }; throws on an unresolvable reloc.
export function link(object, addr, syms) {
  const gp = parseInt(syms.gp ?? '0', 16);
  const text = object.sections.find(s => s.name === '.text');
  const want = retailWords(addr, text.words.length);
  const base = { '.text': addr };
  for (const [k, v] of Object.entries(syms)) if (k.startsWith('.')) base[k] = parseInt(v, 16);
  const rel = text.relocations;
  for (const r of rel) {
    const t = r.target;
    if (r.kind !== 'HI16' || t.kind !== 'section' || base[t.section] !== undefined) continue;
    const lo = rel.find(q => q.kind === 'LO16' && q.target.kind === 'section' && q.target.section === t.section && q.target.offset === t.offset);
    if (!lo) continue;
    const hi = want[r.offset >> 2] & 0xFFFF, l = (want[lo.offset >> 2] << 16) >> 16;
    base[t.section] = (((hi << 16) + l - t.offset) >>> 0);
  }
  // A section-relative target inside a global the source *defines* (C++ puts those in .sdata/.data
  // rather than common): resolve it through that symbol's syms.json address.
  const defined = (object.symbols || []).filter(s => s.section && s.section !== '.text' && syms[s.name] !== undefined);
  const viaSymbol = t => {
    let best;
    for (const s of defined) if (s.section === t.section && s.offset <= t.offset && (!best || s.offset > best.offset)) best = s;
    return best && parseInt(syms[best.name], 16) + (t.offset - best.offset);
  };
  const pieces = {};  // section -> Map(offset -> retail address), when placed piece by piece
  const target = t => {
    if (t.kind === 'section') {
      const p = pieces[t.section]?.get(t.offset | 0);
      if (p !== undefined) return p;
      if ((pieces[t.section] || base[t.section] === undefined) && viaSymbol(t) !== undefined) return viaSymbol(t);
      if (base[t.section] === undefined) throw new Error(`no address for section ${t.section} (add "${t.section}" to syms.json)`);
      return base[t.section] + (t.offset | 0);
    }
    const v = syms[t.name];
    if (v === undefined) throw new Error('unknown symbol ' + t.name);
    return parseInt(v, 16) + (t.addend | 0);
  };
  const apply = (words, relocs) => {
    for (const r of relocs) {
      const i = r.offset >> 2, s = target(r.target);
      let field;
      if (r.kind === 'GPREL16') field = (s - gp) & 0xFFFF;
      else if (r.kind === 'LO16') field = s & 0xFFFF;
      else if (r.kind === 'HI16') field = ((s + 0x8000) >>> 16) & 0xFFFF;
      else if (r.kind === 'MIPS26') field = (s >>> 2) & 0x03FFFFFF;
      else if (r.kind === 'WORD32') field = s >>> 0;
      else throw new Error('unhandled reloc ' + r.kind);
      words[i] = ((words[i] & ~(r.fieldMask >>> 0)) | (field & r.fieldMask)) >>> 0;
    }
    return words;
  };
  // a data section's bytes with its own relocations applied (jump-table entries)
  const sectionBytes = s => {
    const out = Uint8Array.from(s.bytes);
    const dv = new DataView(out.buffer);
    const raw = Array.from({ length: out.length >> 2 }, (_, i) => dv.getUint32(4 * i, true));
    apply(raw, s.relocations || []).forEach((w, i) => dv.setUint32(4 * i, w, true));
    return out;
  };
  // where retail's code puts each piece of a data section: the HI16/LO16 pair or GPREL16 there
  const refs = {};
  for (const r of rel) {
    const t = r.target;
    if (t.kind !== 'section' || t.section === '.text') continue;
    let a;
    if (r.kind === 'GPREL16') a = (gp + ((want[r.offset >> 2] << 16) >> 16)) >>> 0;
    else if (r.kind === 'HI16' || r.kind === 'LO16') {
      const pair = k => r.kind === k ? r : rel.find(q => q.kind === k && q.target.kind === 'section' && q.target.section === t.section && q.target.offset === t.offset);
      const hi = pair('HI16'), lo = pair('LO16');
      if (!hi || !lo) continue;
      a = (((want[hi.offset >> 2] & 0xFFFF) << 16) + ((want[lo.offset >> 2] << 16) >> 16)) >>> 0;
    } else continue;
    const m = (refs[t.section] ??= new Map()), o = t.offset | 0;
    m.set(o, m.has(o) && m.get(o) !== a ? null : a);
  }
  retailBuf ??= readFileSync(RETAIL);
  const inRetail = (a, n) => a >= TEXT_ADDR && a + n <= TEXT_ADDR + retailBuf.length - HDR;
  for (const s of object.sections) {
    const m = refs[s.name];
    if (s.name === '.text' || !s.bytes?.length || !m || [...m.values()].includes(null)) continue;
    // only where one base can't do it: some reference resolves elsewhere than base + offset
    const b = base[s.name];
    if (b !== undefined && [...m].every(([o, a]) => a === b + o)) continue;
    // a global the source defines there is a piece of its own, at its syms.json address (its
    // value isn't checked, as without pieces)
    const globals = new Map(defined.filter(d => d.section === s.name).map(d => [d.offset, parseInt(syms[d.name], 16)]));
    for (const [o, a] of globals) if (m.has(o) && m.get(o) !== a) m.set(o, null);
    if ([...m.values()].includes(null)) continue;
    const bytes = sectionBytes(s);
    const bounds = [...new Set([...m.keys(), ...globals.keys()])].sort((x, y) => x - y);
    const offs = bounds.filter(o => m.has(o) && !globals.has(o));
    const ok = offs.every(o => {
      const k = bounds.indexOf(o);
      let piece = bytes.subarray(o, k + 1 < bounds.length ? bounds[k + 1] : bytes.length);
      // trailing padding: not compared, but a string's NUL is
      let n = piece.length;
      while (n > 0 && piece[n - 1] === 0) n--;
      const need = n < piece.length ? n + 1 : n, a = m.get(o);
      if (!inRetail(a, need)) return false;
      const off = a - TEXT_ADDR + HDR;
      for (let i = 0; i < need; i++) if (retailBuf[off + i] !== piece[i]) return false;
      return true;
    });
    if (ok) {
      for (const [o, a] of globals) m.set(o, a);
      pieces[s.name] = m;
    }
  }
  const words = apply(dmpsx(Array.from(text.words)), rel);
  const data = [];
  for (const s of object.sections) {
    if (s.name === '.text' || !s.bytes?.length) continue;
    if (pieces[s.name]) {
      const n = pieces[s.name].size;
      data.push({ section: s.name, base: Math.min(...pieces[s.name].values()), same: n, total: n, pieces: true });
      continue;
    }
    if (base[s.name] === undefined) continue;
    // compiler-emitted data (a vtable, a constant initializer) whose address can't be inferred
    if (!inRetail(base[s.name], s.bytes.length)) {
      data.push({ section: s.name, base: base[s.name], same: 0, total: 0, skipped: true });
      continue;
    }
    const bytes = sectionBytes(s), dv = new DataView(bytes.buffer);
    const got = Array.from({ length: bytes.length >> 2 }, (_, i) => dv.getUint32(4 * i, true));
    const exp = retailWords(base[s.name], got.length);
    data.push({ section: s.name, base: base[s.name], same: got.filter((w, i) => w === exp[i]).length, total: got.length });
  }
  return { words, data };
}

// Alignment-aware score: longest common subsequence of instruction words. The positional N/M score
// collapses when one extra/missing word shifts everything after it; this one doesn't.
export function alignedScore(got, want) {
  const n = got.length, m = want.length;
  let prev = new Uint16Array(m + 1), cur = new Uint16Array(m + 1);
  for (let i = 1; i <= n; i++) {
    for (let j = 1; j <= m; j++)
      cur[j] = got[i - 1] === want[j - 1] ? prev[j - 1] + 1 : Math.max(prev[j], cur[j - 1]);
    [prev, cur] = [cur, prev];
  }
  return prev[m];
}
