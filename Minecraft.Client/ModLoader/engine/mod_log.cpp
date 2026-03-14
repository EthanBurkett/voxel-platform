#include "mod_log.h"
#include "stdafx.h"
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <mutex>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

static std::mutex s_logMutex;
static FILE *s_logFile = nullptr;
static bool s_logPathTried = false;

static const char *GetLogPath()
{
    static char s_path[1024] = {};
    if (s_path[0] != '\0')
    {
        return s_path;
    }
#ifdef _WIN32
    if (GetModuleFileNameA(nullptr, s_path, sizeof(s_path)) != 0)
    {
        char *p = strrchr(s_path, '\\');
        if (p)
        {
            *(p + 1) = '\0';
            size_t len = strlen(s_path);
            if (len + 8 < sizeof(s_path))
            {
                strcat(s_path, "mods.log");
                return s_path;
            }
        }
    }
#endif
    return "mods.log";
}

static void EnsureLogOpen()
{
    if (s_logFile || s_logPathTried)
    {
        return;
    }
    s_logPathTried = true;
    const char *path = GetLogPath();
    s_logFile = fopen(path, "a");
    if (s_logFile)
    {
        time_t now = time(nullptr);
        struct tm t;
#ifdef _WIN32
        if (localtime_s(&t, &now) == 0)
        {
#else
        if (localtime_r(&now, &t) != nullptr)
        {
#endif
            char buf[64];
            strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &t);
            fprintf(s_logFile, "\n--- ModLoader log %s ---\n", buf);
            fflush(s_logFile);
        }
    }
}

void ModLogMessage(const char *msg)
{
    if (!msg)
    {
        return;
    }
    std::lock_guard<std::mutex> lock(s_logMutex);
    EnsureLogOpen();
    size_t len = strlen(msg);
    int needNewline = (len == 0 || msg[len - 1] != '\n');
    if (s_logFile)
    {
        time_t now = time(nullptr);
        struct tm t;
#ifdef _WIN32
        if (localtime_s(&t, &now) == 0)
        {
#else
        if (localtime_r(&now, &t) != nullptr)
        {
#endif
            char buf[32];
            strftime(buf, sizeof(buf), "%H:%M:%S", &t);
            fprintf(s_logFile, "[%s] %s", buf, msg);
            if (needNewline)
            {
                fputc('\n', s_logFile);
            }
            fflush(s_logFile);
        }
        else
        {
            fprintf(s_logFile, "%s", msg);
            if (needNewline)
            {
                fputc('\n', s_logFile);
            }
            fflush(s_logFile);
        }
    }
#ifdef _WIN32
    std::string dbg = std::string("[ModLoader] ") + msg;
    if (needNewline)
    {
        dbg += '\n';
    }
    OutputDebugStringA(dbg.c_str());
#endif
}

void ModLog(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char buf[4096];
    int n = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    if (n <= 0)
    {
        return;
    }
    if (n >= (int)sizeof(buf))
    {
        buf[sizeof(buf) - 1] = '\0';
    }
    ModLogMessage(buf);
}
