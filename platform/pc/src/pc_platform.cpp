/* Files, folders, the clock and dialogs on Windows and on POSIX systems (see pc/platform.h). */
#ifdef _WIN32
#include <windows.h>
#else
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
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

#include "pc/platform.h"

namespace pc::os {

void PathFix(char *path) {
    for (; *path; path++)
        if (*path == '\\' || *path == '/')
            *path = PC_SEPC;
}

void MakeDirs(const char *path) {
    char p[MAX_PATH], *s;
    snprintf(p, sizeof p, "%s", path);
    PathFix(p);
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

bool IsFile(const char *path) {
    DWORD a = GetFileAttributesA(path);
    return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
}

bool Exists(const char *path) { return GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES; }

long long FileSize(const char *path) {
    WIN32_FILE_ATTRIBUTE_DATA a;
    if (!GetFileAttributesExA(path, GetFileExInfoStandard, &a))
        return 0;
    return (long long)a.nFileSizeHigh << 32 | a.nFileSizeLow;
}

bool RenameOver(const char *from, const char *to) {
    return MoveFileExA(from, to, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
}

void RemoveFile(const char *path) { DeleteFileA(path); }

bool FullPath(const char *path, char *out, int n) {
    DWORD r = GetFullPathNameA(path, (DWORD)n, out, NULL);
    return r && r < (DWORD)n;
}

void ExeDir(char *out, int n) {
    char *slash;
    GetModuleFileNameA(NULL, out, (DWORD)n);
    slash = strrchr(out, '\\');
    if (slash)
        *slash = 0;
}

struct DirList::Impl {
    HANDLE h = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAA fd{};
    bool first = false;
};

DirList::DirList(const char *dir) : m_impl(new Impl) {
    char pat[MAX_PATH];
    snprintf(pat, sizeof pat, "%s\\*", dir);
    m_impl->h = FindFirstFileA(pat, &m_impl->fd);
    if (m_impl->h == INVALID_HANDLE_VALUE) {
        m_impl.reset();
        return;
    }
    m_impl->first = true;
}

DirList::~DirList() {
    if (m_impl)
        FindClose(m_impl->h);
}

bool DirList::Next(DirEntry *e) {
    Impl *d = m_impl.get();
    for (;;) {
        if (!d->first && !FindNextFileA(d->h, &d->fd))
            return false;
        d->first = false;
        if (!strcmp(d->fd.cFileName, ".") || !strcmp(d->fd.cFileName, ".."))
            continue;
        snprintf(e->name, sizeof e->name, "%s", d->fd.cFileName);
        e->isDir = (d->fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        e->mtime = (long long)d->fd.ftLastWriteTime.dwHighDateTime << 32 |
                   d->fd.ftLastWriteTime.dwLowDateTime;
        return true;
    }
}

int64_t ClockNow() {
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return t.QuadPart;
}

int64_t ClockFreq() {
    LARGE_INTEGER f;
    QueryPerformanceFrequency(&f);
    return f.QuadPart;
}

void SleepMs(unsigned ms) { Sleep(ms); }

void ErrorBox(const char *msg) {
    MessageBoxA(NULL, msg, "Tony Hawk's Pro Skater", MB_ICONERROR | MB_OK);
}

void Exit(int code) { ExitProcess((UINT)code); }

bool Readable(const void *p, unsigned n) { return !IsBadReadPtr(p, n); }

void *AllocExec(unsigned size) {
    return VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
}

#else /* POSIX */

bool IsFile(const char *path) {
    struct stat st;
    return !stat(path, &st) && !S_ISDIR(st.st_mode);
}

bool Exists(const char *path) {
    struct stat st;
    return !stat(path, &st);
}

long long FileSize(const char *path) {
    struct stat st;
    return stat(path, &st) ? 0 : (long long)st.st_size;
}

bool RenameOver(const char *from, const char *to) {
    int fd = open(from, O_RDONLY);
    if (fd >= 0) {
        fsync(fd); /* the data is on disk before the name points at it */
        close(fd);
    }
    return rename(from, to) == 0;
}

void RemoveFile(const char *path) { unlink(path); }

bool FullPath(const char *path, char *out, int n) {
    char buf[PATH_MAX];
    if (!realpath(path, buf) || (int)strlen(buf) >= n)
        return false;
    strcpy(out, buf);
    return true;
}

void ExeDir(char *out, int n) {
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

struct DirList::Impl {
    DIR *d = nullptr;
    char path[MAX_PATH] = "";
};

DirList::DirList(const char *dir) : m_impl(new Impl) {
    m_impl->d = opendir(dir);
    if (!m_impl->d) {
        m_impl.reset();
        return;
    }
    snprintf(m_impl->path, sizeof m_impl->path, "%s", dir);
}

DirList::~DirList() {
    if (m_impl)
        closedir(m_impl->d);
}

bool DirList::Next(DirEntry *e) {
    struct dirent *de;
    while ((de = readdir(m_impl->d))) {
        char full[MAX_PATH + 260];
        struct stat st;
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
            continue;
        snprintf(full, sizeof full, "%s/%s", m_impl->path, de->d_name);
        if (stat(full, &st))
            continue;
        snprintf(e->name, sizeof e->name, "%s", de->d_name);
        e->isDir = S_ISDIR(st.st_mode);
        e->mtime = (long long)st.st_mtim.tv_sec * 1000000000LL + st.st_mtim.tv_nsec;
        return true;
    }
    return false;
}

int64_t ClockNow() {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (int64_t)t.tv_sec * 1000000000 + t.tv_nsec;
}

int64_t ClockFreq() { return 1000000000; }

void SleepMs(unsigned ms) {
    struct timespec t;
    t.tv_sec = ms / 1000;
    t.tv_nsec = (long)(ms % 1000) * 1000000L;
    while (nanosleep(&t, &t) && errno == EINTR)
        ;
}

void ErrorBox(const char *msg) {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Tony Hawk's Pro Skater", msg, NULL);
}

void Exit(int code) {
    fflush(NULL);
    _exit(code);
}

bool Readable(const void *p, unsigned n) {
    /* write() reports EFAULT instead of faulting */
    static int fds[2] = {-1, -1};
    char sink[64];
    ssize_t w;
    if (fds[0] < 0 && pipe(fds))
        return false;
    if (n > sizeof sink)
        n = sizeof sink;
    w = write(fds[1], p, n);
    if (w > 0)
        (void)!read(fds[0], sink, (size_t)w);
    return w == (ssize_t)n;
}

void *AllocExec(unsigned size) {
    void *p = mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1,
                   0);
    return p == MAP_FAILED ? NULL : p;
}

#endif

} // namespace pc::os
