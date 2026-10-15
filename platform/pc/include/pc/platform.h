/* What differs between Windows and Linux (and other POSIX systems) outside the memory map and
 * the fault handler (pc_main.cpp): paths, files and folders, the clock, message boxes. Stateless,
 * so free functions; the names avoid windows.h's macros (DeleteFile, ReplaceFile, ...). */
#pragma once

#include <stdint.h>
#include <stdio.h>

#include <memory>

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

namespace pc::os {

/* Turn every '\\' and '/' into the system's separator (the game, Manifest.txt and the code
 * spell relative paths with backslashes). */
void PathFix(char *path);

bool IsFile(const char *path); /* a file, not a folder */
bool Exists(const char *path); /* anything */
long long FileSize(const char *path);
/* Create a folder and any missing parents. */
void MakeDirs(const char *path);
/* Rename `from` over `to` (atomically where the system can); true on success. */
bool RenameOver(const char *from, const char *to);
void RemoveFile(const char *path);
/* An absolute, normalised path; true on success. */
bool FullPath(const char *path, char *out, int n);
/* The folder the executable is in. */
void ExeDir(char *out, int n);

/* A monotonic clock: ticks and ticks per second. */
int64_t ClockNow();
int64_t ClockFreq();
void SleepMs(unsigned ms);

/* An error dialog (where there is a desktop to show one). */
void ErrorBox(const char *msg);
/* Leave now, without running exit handlers. */
void Exit(int code);
/* true if `n` bytes at `p` can be read (crash-report stack walks). */
bool Readable(const void *p, unsigned n);
/* Executable memory for small generated stubs. */
void *AllocExec(unsigned size);

/* An entry of a folder listing. */
struct DirEntry {
    char name[260];
    int isDir;
    long long mtime; /* last write, comparable between entries */
};

/* A folder listing ("." and ".." left out), closed when it goes. False (operator bool) when the
 * folder can't be listed. */
class DirList {
public:
    explicit DirList(const char *dir);
    ~DirList();
    DirList(const DirList &) = delete;
    DirList &operator=(const DirList &) = delete;

    explicit operator bool() const { return m_impl != nullptr; }
    /* The next entry; false at the end. */
    bool Next(DirEntry *e);

private:
    struct Impl; /* the system's handle (pc_platform.cpp) */
    std::unique_ptr<Impl> m_impl;
};

} // namespace pc::os
