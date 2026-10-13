/* Files, folders, the clock and dialogs on Windows and on POSIX systems (see pc_platform.h). */
#ifdef _WIN32
#include <windows.h>
#else
#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <SDL.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pc_platform.h"

void PC_PathFix(char *path) {
    for (; *path; path++)
        if (*path == '\\' || *path == '/')
            *path = PC_SEPC;
}

void PC_MakeDirs(const char *path) {
    char p[MAX_PATH], *s;
    snprintf(p, sizeof p, "%s", path);
    PC_PathFix(p);
    /* every parent; the ones that exist (or can't be made, "C:") just fail */
    for (s = strchr(p + 1, PC_SEPC); s; s = strchr(s + 1, PC_SEPC)) {
        *s = 0;
#ifdef _WIN32
        CreateDirectoryA(p, NULL);
#else
        mkdir(p, 0777);
#endif
        *s = PC_SEPC;
    }
#ifdef _WIN32
    CreateDirectoryA(p, NULL);
#else
    mkdir(p, 0777);
#endif
}

#ifdef _WIN32

int PC_FileExists(const char *path) {
    DWORD a = GetFileAttributesA(path);
    return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
}

int PC_PathExists(const char *path) { return GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES; }

long long PC_FileSize(const char *path) {
    WIN32_FILE_ATTRIBUTE_DATA a;
    if (!GetFileAttributesExA(path, GetFileExInfoStandard, &a))
        return 0;
    return (long long)a.nFileSizeHigh << 32 | a.nFileSizeLow;
}

int PC_ReplaceFile(const char *from, const char *to) {
    return MoveFileExA(from, to, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
}

void PC_DeleteFile(const char *path) { DeleteFileA(path); }

int PC_FullPath(const char *path, char *out, int n) {
    DWORD r = GetFullPathNameA(path, (DWORD)n, out, NULL);
    return r && r < (DWORD)n;
}

void PC_ExeDir(char *out, int n) {
    char *slash;
    GetModuleFileNameA(NULL, out, (DWORD)n);
    slash = strrchr(out, '\\');
    if (slash)
        *slash = 0;
}

struct PcDir {
    HANDLE h;
    WIN32_FIND_DATAA fd;
    int first;
};

PcDir *PC_DirOpen(const char *dir) {
    char pat[MAX_PATH];
    PcDir *d = calloc(1, sizeof *d);
    snprintf(pat, sizeof pat, "%s\\*", dir);
    d->h = FindFirstFileA(pat, &d->fd);
    if (d->h == INVALID_HANDLE_VALUE) {
        free(d);
        return NULL;
    }
    d->first = 1;
    return d;
}

int PC_DirNext(PcDir *d, PcDirEntry *e) {
    for (;;) {
        if (!d->first && !FindNextFileA(d->h, &d->fd))
            return 0;
        d->first = 0;
        if (!strcmp(d->fd.cFileName, ".") || !strcmp(d->fd.cFileName, ".."))
            continue;
        snprintf(e->name, sizeof e->name, "%s", d->fd.cFileName);
        e->isDir = (d->fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        e->mtime = (long long)d->fd.ftLastWriteTime.dwHighDateTime << 32 |
                   d->fd.ftLastWriteTime.dwLowDateTime;
        return 1;
    }
}

void PC_DirClose(PcDir *d) {
    if (d) {
        FindClose(d->h);
        free(d);
    }
}

int64_t PC_ClockNow(void) {
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return t.QuadPart;
}

int64_t PC_ClockFreq(void) {
    LARGE_INTEGER f;
    QueryPerformanceFrequency(&f);
    return f.QuadPart;
}

void PC_SleepMs(unsigned ms) { Sleep(ms); }

void PC_ErrorBox(const char *msg) {
    MessageBoxA(NULL, msg, "Tony Hawk's Pro Skater", MB_ICONERROR | MB_OK);
}

void PC_ExitProcess(int code) { ExitProcess((UINT)code); }

int PC_Readable(const void *p, unsigned n) { return !IsBadReadPtr(p, n); }

void *PC_AllocExec(unsigned size) {
    return VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
}

#else /* POSIX */

int PC_FileExists(const char *path) {
    struct stat st;
    return !stat(path, &st) && !S_ISDIR(st.st_mode);
}

int PC_PathExists(const char *path) {
    struct stat st;
    return !stat(path, &st);
}

long long PC_FileSize(const char *path) {
    struct stat st;
    return stat(path, &st) ? 0 : (long long)st.st_size;
}

int PC_ReplaceFile(const char *from, const char *to) {
    int fd = open(from, O_RDONLY);
    if (fd >= 0) {
        fsync(fd); /* the data is on disk before the name points at it */
        close(fd);
    }
    return rename(from, to) == 0;
}

void PC_DeleteFile(const char *path) { unlink(path); }

int PC_FullPath(const char *path, char *out, int n) {
    char buf[PATH_MAX];
    if (!realpath(path, buf) || (int)strlen(buf) >= n)
        return 0;
    strcpy(out, buf);
    return 1;
}

void PC_ExeDir(char *out, int n) {
    ssize_t len = readlink("/proc/self/exe", out, (size_t)n - 1);
    char *slash;
    if (len <= 0) {
        snprintf(out, n, ".");
        return;
    }
    out[len] = 0;
    slash = strrchr(out, '/');
    if (slash)
        *slash = 0;
}

struct PcDir {
    DIR *d;
    char path[MAX_PATH];
};

PcDir *PC_DirOpen(const char *dir) {
    PcDir *d = calloc(1, sizeof *d);
    d->d = opendir(dir);
    if (!d->d) {
        free(d);
        return NULL;
    }
    snprintf(d->path, sizeof d->path, "%s", dir);
    return d;
}

int PC_DirNext(PcDir *d, PcDirEntry *e) {
    struct dirent *de;
    while ((de = readdir(d->d))) {
        char full[MAX_PATH + 260];
        struct stat st;
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
            continue;
        snprintf(full, sizeof full, "%s/%s", d->path, de->d_name);
        if (stat(full, &st))
            continue;
        snprintf(e->name, sizeof e->name, "%s", de->d_name);
        e->isDir = S_ISDIR(st.st_mode);
        e->mtime = (long long)st.st_mtim.tv_sec * 1000000000LL + st.st_mtim.tv_nsec;
        return 1;
    }
    return 0;
}

void PC_DirClose(PcDir *d) {
    if (d) {
        closedir(d->d);
        free(d);
    }
}

int64_t PC_ClockNow(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (int64_t)t.tv_sec * 1000000000 + t.tv_nsec;
}

int64_t PC_ClockFreq(void) { return 1000000000; }

void PC_SleepMs(unsigned ms) {
    struct timespec t;
    t.tv_sec = ms / 1000;
    t.tv_nsec = (long)(ms % 1000) * 1000000L;
    while (nanosleep(&t, &t) && errno == EINTR)
        ;
}

void PC_ErrorBox(const char *msg) {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Tony Hawk's Pro Skater", msg, NULL);
}

void PC_ExitProcess(int code) {
    fflush(NULL);
    _exit(code);
}

int PC_Readable(const void *p, unsigned n) {
    /* write() reports EFAULT instead of faulting */
    static int fds[2] = {-1, -1};
    char sink[64];
    ssize_t w;
    if (fds[0] < 0 && pipe(fds))
        return 0;
    if (n > sizeof sink)
        n = sizeof sink;
    w = write(fds[1], p, n);
    if (w > 0)
        (void)!read(fds[0], sink, (size_t)w);
    return w == (ssize_t)n;
}

void *PC_AllocExec(unsigned size) {
    void *p = mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1,
                   0);
    return p == MAP_FAILED ? NULL : p;
}

#endif
