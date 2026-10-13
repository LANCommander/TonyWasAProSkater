/* What differs between Windows and Linux (and other POSIX systems) outside the memory map and
 * the fault handler (pc_main.c): paths, files and folders, the clock, message boxes. */
#ifndef PC_PLATFORM_H
#define PC_PLATFORM_H

#include <stdint.h>
#include <stdio.h>

#ifdef _WIN32
#ifndef MAX_PATH
#define MAX_PATH 260 /* as windows.h has it */
#endif
#define PC_SEP "\\"
#define PC_SEPC '\\'
#define PC_EXE_NAME "thps.exe"
#define PC_FSEEK _fseeki64
#define PC_FTELL _ftelli64
#else
#include <limits.h>
#include <strings.h>
#ifndef MAX_PATH
#define MAX_PATH 4096
#endif
#define PC_SEP "/"
#define PC_SEPC '/'
#define PC_EXE_NAME "thps"
#define PC_FSEEK fseeko
#define PC_FTELL ftello
#define _stricmp strcasecmp
#define _strnicmp strncasecmp
#define _strdup strdup
#endif

/* An assembler name for a C symbol (PE prefixes C names with '_', ELF doesn't). */
#ifdef _WIN32
#define PC_ASM_SYM(name) "_" name
#else
#define PC_ASM_SYM(name) name
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Turn every '\\' and '/' into the system's separator (the game, Manifest.txt and the code
 * spell relative paths with backslashes). */
void PC_PathFix(char *path);

int PC_FileExists(const char *path); /* a file, not a folder */
int PC_PathExists(const char *path); /* anything */
long long PC_FileSize(const char *path);
/* Create a folder and any missing parents. */
void PC_MakeDirs(const char *path);
/* Rename `from` over `to` (atomically where the system can); 1 on success. */
int PC_ReplaceFile(const char *from, const char *to);
void PC_DeleteFile(const char *path);
/* An absolute, normalised path; 1 on success. */
int PC_FullPath(const char *path, char *out, int n);
/* The folder the executable is in. */
void PC_ExeDir(char *out, int n);

/* Folder listing ("." and ".." left out). */
typedef struct {
    char name[260];
    int isDir;
    long long mtime; /* last write, comparable between entries */
} PcDirEntry;
typedef struct PcDir PcDir;
PcDir *PC_DirOpen(const char *dir);
int PC_DirNext(PcDir *d, PcDirEntry *e);
void PC_DirClose(PcDir *d);

/* A monotonic clock: ticks and ticks per second. */
int64_t PC_ClockNow(void);
int64_t PC_ClockFreq(void);
void PC_SleepMs(unsigned ms);

/* An error dialog (where there is a desktop to show one). */
void PC_ErrorBox(const char *msg);
/* Leave now, without running exit handlers. */
void PC_ExitProcess(int code);
/* 1 if `n` bytes at `p` can be read (crash-report stack walks). */
int PC_Readable(const void *p, unsigned n);
/* Executable memory for small generated stubs. */
void *PC_AllocExec(unsigned size);

#ifdef __cplusplus
}
#endif

#endif
