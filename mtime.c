#define _WIN32_WINNT _WIN32_WINNT_VISTA
#define WIN32_LEAN_AND_MEAN

#define STRICT 1
#define STRICT_TYPED_ITEMIDS
#define STRICT_CONST
#define PATHCCH_NO_DEPRECATE

#include <Windows.h>
#include <shellapi.h>
#include <objbase.h>
#include <shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

FILETIME UlongLong2Ft(ULARGE_INTEGER ul)
{
    FILETIME ft = { .dwHighDateTime = ul.HighPart, .dwLowDateTime = ul.LowPart };
    return ft;
}

ULONGLONG Ft2UlongLong(FILETIME ft)
{
    ULARGE_INTEGER ul = { .HighPart = ft.dwHighDateTime, .LowPart = ft.dwLowDateTime };
    return ul.QuadPart;
}

void printTime(LPCSTR header, FILETIME ft)
{
    ULONGLONG ul = Ft2UlongLong(ft) / (10 * 1000);

    ULONG realMs = ul % 1000;
    ULONG realSec = (ul / 1000) % 60;
    ULONGLONG realMin = ul / (60 * 1000);
    fprintf(stderr, "%s: %llum%u.%03us\n", header, realMin, (UINT)realSec, realMs);
}

#define COMMANDLINE_MAX 32767 
int wmain(int argc, wchar_t* argv[])
{
    if (FAILED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)))
        return EXIT_FAILURE;

    bool verbose = false;
    wchar_t* param = calloc(COMMANDLINE_MAX, sizeof(wchar_t));
    if (param == NULL) {
        return EXIT_FAILURE;
    }
    wchar_t* lpFileName = NULL;

    bool fBeforeCommand = true;

    for (int i = 1; i < argc; i++)
    {
        verbose ? fprintf(stderr, "mtime: argv[%d]:%S\n", i, argv[i]) : 0;
        if (fBeforeCommand)
        {
            lpFileName = argv[i];
            //SearchPath(NULL, argv[i], NULL, MAX_PATH, lpFileName, NULL);
            verbose ? fprintf(stderr, "%S\n", lpFileName) : 0;
            fBeforeCommand = false;
            continue;
        }

        wcscat_s(param, COMMANDLINE_MAX, L" ");
        wcscat_s(param, COMMANDLINE_MAX, L"\"");
        wcscat_s(param, COMMANDLINE_MAX, argv[i]);
        wcscat_s(param, COMMANDLINE_MAX, L"\"");
    }

    SHELLEXECUTEINFO sei =
    {
        .cbSize = sizeof(sei),
        .fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NO_CONSOLE | SEE_MASK_NOASYNC | SEE_MASK_FLAG_NO_UI,
        .lpFile = lpFileName,
        .lpParameters = param,
        .lpVerb = L"open",
        .nShow = SW_NORMAL,
    };

    if (FALSE == ShellExecuteEx(&sei))
    {
        return EXIT_FAILURE;
    }

    // ignore Ctrl-C
    SetConsoleCtrlHandler(NULL, TRUE);

    if (sei.hProcess)
    {
        WaitForSingleObject(sei.hProcess, INFINITE);
        SIZE_T MinWs = { 0 }, MaxWs = { 0 };

        if (GetProcessWorkingSetSize(sei.hProcess, &MinWs, &MaxWs))
        {
            fprintf(stderr, "minm: %zu\n", MinWs / 1024);
            fprintf(stderr, "maxm: %zu\n", MaxWs / 1024);
        }
        else
        {
            fprintf(stderr, "mem statics not available.\n");
        }

        IO_COUNTERS ioCounters = { 0 };
        if (GetProcessIoCounters(sei.hProcess, &ioCounters))
        {
            fprintf(stderr, "i/o op: %llu %llu %llu\n",
                ioCounters.ReadOperationCount,
                ioCounters.WriteOperationCount,
                ioCounters.OtherOperationCount);
            fprintf(stderr, "i/o transfer: %lluk %lluk %lluk\n",
                ioCounters.ReadTransferCount / 1024,
                ioCounters.WriteTransferCount / 1024,
                ioCounters.OtherTransferCount / 1024);
        }
        else
        {
            fprintf(stderr, "i/o statics not available.\n");
        }

        FILETIME ftCreationTime = { 0 };
        FILETIME ftExitTime = { 0 };
        FILETIME ftKernelTime = { 0 };
        FILETIME ftUserTime = { 0 };
        if (GetProcessTimes(sei.hProcess, &ftCreationTime, &ftExitTime, &ftKernelTime, &ftUserTime))
        {
            ULONGLONG elapsedTime = Ft2UlongLong(ftExitTime) - Ft2UlongLong(ftCreationTime);
            ULARGE_INTEGER ulelapsedTime = { .QuadPart = elapsedTime };
            printTime("real", UlongLong2Ft(ulelapsedTime));
            printTime("user", ftUserTime);
            printTime("sys ", ftKernelTime);
        }
        else
        {
            fprintf(stderr, "time statics not available.\n");
        }

        DWORD exitCode = { 0 };
        GetExitCodeProcess(sei.hProcess, &exitCode);
        verbose ? fprintf(stderr, "mtime: exit code: %u\n", exitCode) : 0;

        CloseHandle(sei.hProcess);
        ExitProcess(exitCode);
    }
    return EXIT_FAILURE;
}
