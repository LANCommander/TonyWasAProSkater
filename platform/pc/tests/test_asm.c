/* Differential test for the C that replaces retail MIPS: hand-written assembly (decomp/src/m3dasm,
 * decomp/reimpl.tsv) and Psy-Q library code (src/hle_libgte.c ...). Each snapshot the port
 * recorded at a function's entry (THPS_SNAP, src/pc_snap.c) is replayed twice: through the retail
 * MIPS in tests/mips_ref.c (COP2 on the software GTE) and through the C, from the same RAM,
 * scratchpad, GTE registers, arguments and native stack (the arguments often point at a caller's
 * locals). The two must leave the same RAM (outside the code and the MIPS stack frames),
 * scratchpad, native stack, GTE registers and return value.
 *
 *   python platform/pc/tools/build_test_asm.py      (-> build/pc/test_asm.exe)
 *   build/pc/test_asm.exe [-v] [--exe SLUS_008.60] [--only 8008b670 ...] snapshot.snp|dir ...
 * (default: every snapshot under build/pc/snaps)
 *
 * The C runs on PS1 memory mapped at its own addresses, as in the port, so the test is a 32-bit
 * large-address-aware program. */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mips_ref.h"
#include "pc_gte.h"

#define RAM_BASE 0x80000000u
#define SCRATCH_BASE 0x1f800000u
#define CODE_START 0x80010000u
#define CODE_END 0x8009d0e0u /* first byte of .rodata */
#define STACK_WINDOW 0x10000u /* below the snapshot's sp: the MIPS stack frames */
#define NARGS 12

volatile uint32_t pc_heartbeat; /* pc_gte.c */
uint32_t pc_ram_top = 0x80200000u;

typedef uint32_t (*Fn12)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
                         uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
typedef struct {
	uint32_t addr;
	void *fn;
	const char *name;
	int ret; /* compare the return value */
	/* 32-bit words the C may leave different (scratch temporaries nothing reads), 0-terminated */
	uint32_t ignore[8];
	/* pointer words the C keeps in KSEG0 where the retail code leaves a KUSEG (bit 31 clear)
	 * value: compared with bit 31 set on both sides, 0-terminated */
	uint32_t kseg[4];
} AsmImpl;

#include "test_asm_impls.h" /* generated: AsmImpl asm_impls[] */

static int verbose;
static uint8_t *s_exe;   /* the retail image, 0x80010000.. */
static uint32_t s_exeSize;

/* The port's native-data lookup (pc_main.c; hle_libgte.c's tables): in a snapshot every global is
 * at its retail address */
void *PC_Data(uint32_t addr) { return (void *)(uintptr_t)addr; }

static const AsmImpl *Find(uint32_t addr) {
	const AsmImpl *p;
	for (p = asm_impls; p->addr; p++)
		if (p->addr == addr)
			return p;
	return NULL;
}

static int Ignored(const AsmImpl *im, uint32_t a) {
	int i;
	for (i = 0; i < 8 && im->ignore[i]; i++)
		if ((a & ~3u) == im->ignore[i])
			return 1;
	return 0;
}

static int Kseg(const AsmImpl *im, uint32_t a) {
	int i;
	for (i = 0; i < 4 && im->kseg[i]; i++)
		if ((a & ~3u) == im->kseg[i])
			return 1;
	return 0;
}

static void MapMemory(void) {
	void *r = VirtualAlloc((void *)(uintptr_t)RAM_BASE, 0x800000, MEM_RESERVE | MEM_COMMIT,
	                       PAGE_READWRITE);
	void *s = VirtualAlloc((void *)(uintptr_t)SCRATCH_BASE, 0x1000, MEM_RESERVE | MEM_COMMIT,
	                       PAGE_READWRITE);
	if (r != (void *)(uintptr_t)RAM_BASE || s != (void *)(uintptr_t)SCRATCH_BASE) {
		fprintf(stderr, "cannot map PS1 memory (needs a 32-bit large-address-aware build)\n");
		exit(2);
	}
}

static int LoadExe(const char *path) {
	FILE *f = fopen(path, "rb");
	uint8_t hdr[0x800];
	if (!f)
		return 0;
	if (fread(hdr, 1, sizeof hdr, f) != sizeof hdr || memcmp(hdr, "PS-X EXE", 8)) {
		fclose(f);
		return 0;
	}
	memcpy(&s_exeSize, hdr + 0x1c, 4);
	s_exe = malloc(s_exeSize);
	if (fread(s_exe, 1, s_exeSize, f) != s_exeSize) {
		fclose(f);
		return 0;
	}
	fclose(f);
	return 1;
}

typedef struct {
	uint32_t addr, args[NARGS], ram, sp;
	uint8_t *mem, *scratch, *gte;
	uint32_t stack_base, stack_size; /* the game's native stack above the arguments */
	uint8_t *stack;
} Snap;

static int ReadSnap(const char *path, Snap *s) {
	uint32_t hdr[8 + NARGS];
	FILE *f = fopen(path, "rb");
	int gs = PC_GteStateSize();
	memset(s, 0, sizeof *s);
	if (!f)
		return 0;
	if (fread(hdr, 4, 8 + NARGS, f) != 8 + NARGS || memcmp(hdr, "TSNP", 4) || hdr[1] != 2 ||
	    hdr[3] != NARGS || (int)hdr[5] != gs) {
		fprintf(stderr, "%s: not a snapshot of this port's version\n", path);
		fclose(f);
		return 0;
	}
	s->addr = hdr[2];
	s->ram = hdr[4];
	s->sp = hdr[6];
	memcpy(s->args, &hdr[8], sizeof s->args);
	s->mem = malloc(s->ram);
	s->scratch = malloc(1024);
	s->gte = malloc((size_t)gs);
	if (fread(s->mem, 1, s->ram, f) != s->ram || fread(s->scratch, 1, 1024, f) != 1024 ||
	    fread(s->gte, 1, (size_t)gs, f) != (size_t)gs || fread(&s->stack_base, 4, 1, f) != 1 ||
	    fread(&s->stack_size, 4, 1, f) != 1 || !(s->stack = malloc(s->stack_size)) ||
	    fread(s->stack, 1, s->stack_size, f) != s->stack_size) {
		fprintf(stderr, "%s: truncated\n", path);
		fclose(f);
		return 0;
	}
	fclose(f);
	return 1;
}

static void FreeSnap(Snap *s) {
	free(s->stack);
	free(s->mem);
	free(s->scratch);
	free(s->gte);
}

static int s_pass, s_fail, s_skip;
static uint8_t *s_stack;
static uint32_t s_stackBase, s_stackSize;

static void RunSnap(const char *path) {
	Snap s;
	MRef m;
	const AsmImpl *im;
	uint32_t v_mips, v_c, a, lo, hi;
	int gs = PC_GteStateSize(), diffs = 0, i;
	uint8_t *gte_mips = malloc((size_t)gs), *gte_c = malloc((size_t)gs), *stack_mips = NULL;

	if (!ReadSnap(path, &s)) {
		s_fail++;
		return;
	}
	im = Find(s.addr);
	/* InitGeom patches the BIOS exception handler (syscall, COP0): mips_ref can't run it */
	if (s.addr == 0x800971a4u || s.addr == 0x800971bcu)
		im = NULL;
	if (!im || !im->fn) {
		if (verbose)
			printf("%s: no C for %08x, skipped\n", path, s.addr);
		s_skip++;
		FreeSnap(&s);
		return;
	}
	if (s.ram != 0x200000u) {
		printf("%s: %u KB of RAM (Dreamcast data?): only 2 MB snapshots are supported\n", path,
		       s.ram >> 10);
		s_skip++;
		FreeSnap(&s);
		return;
	}

	/* the game's native stack, at its own address here too */
	if (!s_stack || s.stack_base < s_stackBase ||
	    s.stack_base + s.stack_size > s_stackBase + s_stackSize) {
		if (s_stack)
			VirtualFree(s_stack, 0, MEM_RELEASE);
		s_stackBase = s.stack_base & ~0xffffu;
		s_stackSize = ((s.stack_base + s.stack_size + 0xffffu) & ~0xffffu) - s_stackBase;
		s_stack = VirtualAlloc((void *)(uintptr_t)s_stackBase, s_stackSize,
		                       MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		if (s_stack != (void *)(uintptr_t)s_stackBase) {
			printf("%s: can't map the game's stack at %08x here, skipped\n", path, s.stack_base);
			s_stack = NULL;
			s_skip++;
			FreeSnap(&s);
			free(gte_mips);
			free(gte_c);
			return;
		}
	}
	stack_mips = malloc(s.stack_size);
	memcpy(stack_mips, s.stack, s.stack_size);

	/* the retail MIPS, with the original code back where the port put its trampolines */
	memset(&m, 0, sizeof m);
	m.unaligned_ok = 1; /* native callers' locals are only 2-byte aligned */
	m.ext = stack_mips;
	m.ext_base = s.stack_base;
	m.ext_size = s.stack_size;
	m.ram = malloc(MREF_RAM_SIZE);
	memcpy(m.ram, s.mem, s.ram);
	memcpy(m.ram + (CODE_START - RAM_BASE), s_exe, CODE_END - CODE_START);
	memcpy(m.scratch, s.scratch, 1024);
	PC_GteLoadState(s.gte);
	v_mips = MRef_CallArgs(&m, s.addr, s.args, NARGS, s.sp);
	PC_GteSaveState(gte_mips);
	if (m.fault) {
		printf("%s: the retail code faulted (pc %08x)\n", path, m.pc);
		s_fail++;
		goto done;
	}

	/* the C */
	memcpy((void *)(uintptr_t)RAM_BASE, s.mem, s.ram);
	memcpy((void *)(uintptr_t)SCRATCH_BASE, s.scratch, 1024);
	memcpy((void *)(uintptr_t)s.stack_base, s.stack, s.stack_size);
	PC_GteLoadState(s.gte);
	v_c = ((Fn12)im->fn)(s.args[0], s.args[1], s.args[2], s.args[3], s.args[4], s.args[5],
	                     s.args[6], s.args[7], s.args[8], s.args[9], s.args[10], s.args[11]);
	PC_GteSaveState(gte_c);

	if (im->ret && v_mips != v_c) {
		printf("%s: returned %08x, retail %08x\n", path, v_c, v_mips);
		diffs++;
	}
	lo = (s.sp - STACK_WINDOW) & ~3u;
	hi = s.sp;
	for (a = RAM_BASE; a < RAM_BASE + s.ram; a += 4) {
		uint32_t x, y;
		if (a >= CODE_START && a < CODE_END)
			continue;
		if (a >= lo && a < hi)
			continue;
		memcpy(&x, m.ram + (a - RAM_BASE), 4);
		memcpy(&y, (void *)(uintptr_t)a, 4);
		if (Kseg(im, a)) {
			x |= 0x80000000u;
			y |= 0x80000000u;
		}
		if (x != y && !Ignored(im, a)) {
			uint32_t was;
			memcpy(&was, s.mem + (a - RAM_BASE), 4);
			if (diffs < 16 || verbose)
				printf("%s: RAM %08x = %08x, retail %08x (was %08x)\n", path, a, y, x, was);
			diffs++;
		}
	}
	for (i = 0; i < 1024; i += 4) {
		uint32_t x, y;
		memcpy(&x, m.scratch + i, 4);
		memcpy(&y, (uint8_t *)(uintptr_t)SCRATCH_BASE + i, 4);
		if (x != y && !Ignored(im, SCRATCH_BASE + (uint32_t)i)) {
			uint32_t was;
			memcpy(&was, s.scratch + i, 4);
			if (diffs < 16 || verbose)
				printf("%s: scratch %08x = %08x, retail %08x (was %08x)\n", path,
				       SCRATCH_BASE + (uint32_t)i, y, x, was);
			diffs++;
		}
	}
	for (i = 0; i < (int)s.stack_size; i += 4) {
		uint32_t x, y, a = s.stack_base + (uint32_t)i;
		memcpy(&x, stack_mips + i, 4);
		memcpy(&y, (void *)(uintptr_t)a, 4);
		if (x != y) {
			uint32_t was;
			memcpy(&was, s.stack + i, 4);
			if (diffs < 16 || verbose)
				printf("%s: native stack %08x = %08x, retail %08x (was %08x)\n", path, a, y, x,
				       was);
			diffs++;
		}
	}
	if (memcmp(gte_mips, gte_c, (size_t)gs)) {
		/* name the registers: read them back through the register interface */
		int r;
		for (r = 0; r < 64; r++) {
			uint32_t x, y;
			PC_GteLoadState(gte_mips);
			x = r < 32 ? PC_GteGetData(r) : PC_GteGetCtrl(r - 32);
			PC_GteLoadState(gte_c);
			y = r < 32 ? PC_GteGetData(r) : PC_GteGetCtrl(r - 32);
			if (x != y) {
				printf("%s: GTE %s %d = %08x, retail %08x\n", path, r < 32 ? "data" : "ctrl",
				       r & 31, y, x);
				diffs++;
			}
		}
	}
	if (diffs) {
		printf("%s: %s FAILED (%d differences)\n", path, im->name, diffs);
		s_fail++;
	} else {
		if (verbose)
			printf("%s: %s ok\n", path, im->name);
		s_pass++;
	}
done:
	free(stack_mips);
	free(m.ram);
	free(gte_mips);
	free(gte_c);
	FreeSnap(&s);
}

static const char *s_only[64]; /* --only addr: just these functions' snapshots */
static int s_nonly;

static int Wanted(const char *file) {
	int i;
	if (!s_nonly)
		return 1;
	for (i = 0; i < s_nonly; i++)
		if (!_strnicmp(file, s_only[i], 8))
			return 1;
	return 0;
}

/* A snapshot, or every snapshot in a directory tree. */
static void RunPath(const char *path) {
	DWORD at = GetFileAttributesA(path);
	if (at != INVALID_FILE_ATTRIBUTES && (at & FILE_ATTRIBUTE_DIRECTORY)) {
		char pat[MAX_PATH], full[MAX_PATH];
		WIN32_FIND_DATAA fd;
		HANDLE h;
		snprintf(pat, sizeof pat, "%s\\*", path);
		h = FindFirstFileA(pat, &fd);
		if (h == INVALID_HANDLE_VALUE)
			return;
		do {
			size_t n = strlen(fd.cFileName);
			if (fd.cFileName[0] == '.')
				continue;
			snprintf(full, sizeof full, "%s\\%s", path, fd.cFileName);
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				RunPath(full);
			else if (n > 4 && !_stricmp(fd.cFileName + n - 4, ".snp") && Wanted(fd.cFileName))
				RunSnap(full);
		} while (FindNextFileA(h, &fd));
		FindClose(h);
	} else {
		RunSnap(path);
	}
}

int main(int argc, char **argv) {
	const char *exe = "extern/thps1_retail/SLUS_008.60";
	int i, any = 0;
	MapMemory();
	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-v"))
			verbose = 1;
		else if (!strcmp(argv[i], "--exe") && i + 1 < argc)
			exe = argv[++i];
		else if (!strcmp(argv[i], "--only") && i + 1 < argc && s_nonly < 64)
			s_only[s_nonly++] = argv[++i];
	}
	if (!LoadExe(exe)) {
		fprintf(stderr, "cannot read %s (use --exe)\n", exe);
		return 2;
	}
	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-v"))
			continue;
		if (!strcmp(argv[i], "--exe") || !strcmp(argv[i], "--only")) {
			i++;
			continue;
		}
		RunPath(argv[i]);
		any = 1;
	}
	if (!any)
		RunPath("build/pc/snaps");
	printf("%d passed, %d failed, %d skipped\n", s_pass, s_fail, s_skip);
	return s_fail ? 1 : 0;
}
