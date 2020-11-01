#include "windows_api.h"
#include <unistd.h>
#include <malloc.h>
#include <asm/unistd_64.h>
#include <sys/syscall.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <stdlib.h>

// Typedefs
typedef uint32_t UINT;
typedef int64_t LONG_PTR, * PLONG_PTR;
typedef unsigned char BYTE;
typedef uint16_t WORD;
typedef wchar_t WCHAR;
typedef WCHAR* PWCHAR, * LPWCH, * PWCH;
typedef WCHAR* NWPSTR, * LPWSTR, * PWSTR;
typedef const WCHAR* LPCWCH, * PCWCH;
typedef char* NPSTR, * LPSTR, * PSTR;
typedef const char* LPCCH, * PCCH;
typedef BYTE* LPBYTE;
typedef BOOL* LPBOOL;
typedef uint64_t LARGE_INTEGER;
typedef void* HANDLE;
typedef void* PVOID;
typedef int64_t INT_PTR;
typedef INT_PTR(WINAPI* FARPROC)();

typedef struct _FILETIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
} FILETIME, * PFILETIME;

typedef struct _STARTUPINFOW {
    DWORD   cb;
    LPWSTR  lpReserved;
    LPWSTR  lpDesktop;
    LPWSTR  lpTitle;
    DWORD   dwX;
    DWORD   dwY;
    DWORD   dwXSize;
    DWORD   dwYSize;
    DWORD   dwXCountChars;
    DWORD   dwYCountChars;
    DWORD   dwFillAttribute;
    DWORD   dwFlags;
    WORD    wShowWindow;
    WORD    cbReserved2;
    LPBYTE  lpReserved2;
    HANDLE  hStdInput;
    HANDLE  hStdOutput;
    HANDLE  hStdError;
} STARTUPINFOW, * LPSTARTUPINFOW;

// Constants
#define FALSE 0
#define TRUE 1
#define HEAP_ZERO_MEMORY 0x00000008
#define TLS_OUT_OF_INDEXES ((DWORD)0xFFFFFFFF)
#define CP_ACP 0
#define ERROR_INSUFFICIENT_BUFFER 122
#define ERROR_INVALID_PARAMETER 87
#define STD_INPUT_HANDLE ((DWORD)-10)
#define STD_OUTPUT_HANDLE ((DWORD)-11)
#define STD_ERROR_HANDLE ((DWORD)-12)
#define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)
#define FILE_TYPE_CHAR 0x0002

char g_commandLine[] = "linoodle.exe";
WCHAR g_environmentStrings[] = L"ALLUSERSPROFILE=AllUsersProfile\0APPDATA=AppData\0COMPUTERNAME=DESKTOP-ABCDEFG\0LOCALAPPDATA=LocalAppData\0";

// Functions
void* WINAPI HeapAlloc(HANDLE hHeap, uint32_t dwFlags, size_t dwBytes)
{
    if (dwFlags & HEAP_ZERO_MEMORY) {
        return calloc(dwBytes, 1);
    }
    else {
        return malloc(dwBytes);
    }
}

BOOL WINAPI HeapFree(HANDLE hHeap, uint32_t dwFlags, void* lpMem)
{
    free(lpMem);
    return TRUE;
}

size_t WINAPI HeapSize(HANDLE hHeap, DWORD dwFlags, PVOID lpMem)
{
    return malloc_usable_size(lpMem);
}

HANDLE WINAPI GetProcessHeap()
{
    return (HANDLE)0xDEADBEEF;
}

DWORD WINAPI GetCurrentThreadId()
{
    return syscall(__NR_gettid);
}

DWORD WINAPI GetCurrentProcessId()
{
    return getpid();
}

void WINAPI GetSystemTimeAsFileTime(PFILETIME lpSystemTimeAsFileTime)
{
    memset(lpSystemTimeAsFileTime, 0, sizeof(FILETIME));
}

BOOL WINAPI QueryPerformanceCounter(LARGE_INTEGER* lpPerformanceCount)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    *lpPerformanceCount = (now.tv_sec * 1000000000) + now.tv_nsec;
    return TRUE;
}

static_assert(sizeof(pthread_mutex_t) <= 40, "pthread_mutex_t is not large enough"); // 40 == sizeof(CRITICAL_SECTION) on Windows
BOOL WINAPI InitializeCriticalSectionAndSpinCount(PVOID lpCriticalSection, DWORD dwSpinCount)
{
    return pthread_mutex_init(reinterpret_cast<pthread_mutex_t*>(lpCriticalSection), nullptr) == 0;
}

void WINAPI EnterCriticalSection(PVOID lpCriticalSection)
{
    if (pthread_mutex_lock(reinterpret_cast<pthread_mutex_t*>(lpCriticalSection)) != 0) {
        abort();
    }
}

void WINAPI LeaveCriticalSection(PVOID lpCriticalSection)
{
    if (pthread_mutex_unlock(reinterpret_cast<pthread_mutex_t*>(lpCriticalSection)) != 0) {
        abort();
    }
}

void WINAPI DeleteCriticalSection(PVOID lpCriticalSection)
{
    pthread_mutex_destroy(reinterpret_cast<pthread_mutex_t*>(lpCriticalSection));
}

DWORD WINAPI TlsAlloc()
{
    pthread_key_t threadKey;
    if (pthread_key_create(&threadKey, nullptr) == 0) {
        return threadKey;
    }
    else {
        return TLS_OUT_OF_INDEXES;
    }
}

BOOL WINAPI TlsSetValue(DWORD dwTlsIndex, PVOID lpTlsValue)
{
    return pthread_setspecific(dwTlsIndex, lpTlsValue) == 0;
}

PVOID WINAPI TlsGetValue(DWORD dwTlsIndex)
{
    return pthread_getspecific(dwTlsIndex);
}

BOOL WINAPI TlsFree(DWORD dwTlsIndex)
{
    return pthread_key_delete(dwTlsIndex) == 0;
}

char* WINAPI GetCommandLineA()
{
    return g_commandLine;
}

__thread DWORD g_lastError;
DWORD WINAPI GetLastError()
{
    return g_lastError;
}

void WINAPI SetLastError(DWORD dwErrCode)
{
    g_lastError = dwErrCode;
}

void WINAPI GetStartupInfoW(LPSTARTUPINFOW lpStartupInfo)
{
    memset(lpStartupInfo, 0, sizeof(STARTUPINFOW));
}

HANDLE WINAPI GetStdHandle(DWORD nStdHandle)
{
    switch (nStdHandle) {
    case STD_INPUT_HANDLE:
        return (HANDLE)0;
    case STD_OUTPUT_HANDLE:
        return (HANDLE)1;
    case STD_ERROR_HANDLE:
        return (HANDLE)2;
    }

    return INVALID_HANDLE_VALUE;
}

DWORD WINAPI GetFileType(HANDLE hFile)
{
    return FILE_TYPE_CHAR;
}

UINT WINAPI GetACP()
{
    return 65001; // utf-8
}

DWORD WINAPI GetModuleFileNameA(HANDLE hModule, char* lpFilename, DWORD nSize)
{
    if (hModule == nullptr) {
        strncpy(lpFilename, "linoodle.exe", nSize);
        lpFilename[nSize - 1] = 0;
        return strlen(lpFilename);
    }
    else {
        return 0;
    }
}

LPWCH WINAPI GetEnvironmentStringsW()
{
    return g_environmentStrings;
}

BOOL WINAPI FreeEnvironmentStringsW(LPWCH penv)
{
    return TRUE;
}

PVOID WINAPI EncodePointer(PVOID Ptr)
{
    return (PVOID)((uint64_t)(Ptr) ^ 0x5BF0A7F83421AC42);
}

HANDLE WINAPI GetModuleHandleW(PVOID lpModuleName)
{
    if (lpModuleName && memcmp(lpModuleName, L"KERNEL32.DLL", sizeof(L"KERNEL32.DLL")) == 0)
        return (HANDLE)0x41414141;
    return (HANDLE)nullptr;
}

FARPROC WINAPI GetProcAddress(HANDLE hModule, const char* lpProcName)
{
    if (hModule == (HANDLE)0x41414141) {
        return reinterpret_cast<FARPROC>(WindowsAPI::GetInstance().GetFunction("KERNEL32.DLL", lpProcName));
    }

    return nullptr;
}

int32_t WINAPI WideCharToMultiByte(UINT CodePage, DWORD dwFlags, LPCWCH lpWideCharStr, int32_t cchWideChar, LPSTR lpMultiByteStr, int32_t cbMultiByte, LPCCH lpDefaultChar, LPBOOL lpUsedDefaultChar)
{
    if (CodePage != CP_ACP || dwFlags != 0 || cchWideChar == 0) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (cchWideChar != -1) {
        // Ensure the input string is null terminated
        wchar_t* tempBuf = static_cast<wchar_t*>(calloc(cchWideChar + 1, sizeof(wchar_t)));
        memcpy(tempBuf, lpWideCharStr, cchWideChar * 2);

        // Determine how many bytes we need in the output
        mbstate_t state = { 0 };
        const wchar_t* src = tempBuf;
        size_t neededLen = wcsrtombs(nullptr, &src, 0, &state);

        if (cbMultiByte == 0) {
            free(tempBuf);
            return neededLen;
        }
        else if (cbMultiByte < neededLen) {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            free(tempBuf);
            return 0;
        }
        else {
            size_t result = wcsrtombs(lpMultiByteStr, &src, cbMultiByte, &state);
            free(tempBuf);
            return result;
        }
    }
    else {
        mbstate_t state = { 0 };
        size_t neededLen = 1 + wcsrtombs(nullptr, const_cast<const wchar_t**>(&lpWideCharStr), 0, &state);

        if (cbMultiByte == 0) {
            return neededLen;
        }
        else if (cbMultiByte < neededLen) {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return 0;
        }
        else {
            return wcsrtombs(lpMultiByteStr, const_cast<const wchar_t**>(&lpWideCharStr), cbMultiByte, &state);
        }
    }
}

WindowsAPI::WindowsAPI()
{
    // Fill in the API map
    m_apiMap["KERNEL32.DLL"]["HeapAlloc"] = reinterpret_cast<void*>(HeapAlloc);
    m_apiMap["KERNEL32.DLL"]["HeapFree"] = reinterpret_cast<void*>(HeapFree);
    m_apiMap["KERNEL32.DLL"]["GetProcessHeap"] = reinterpret_cast<void*>(GetProcessHeap);
    m_apiMap["KERNEL32.DLL"]["GetSystemTimeAsFileTime"] = reinterpret_cast<void*>(GetSystemTimeAsFileTime);
    m_apiMap["KERNEL32.DLL"]["GetCurrentThreadId"] = reinterpret_cast<void*>(GetCurrentThreadId);
    m_apiMap["KERNEL32.DLL"]["GetCurrentProcessId"] = reinterpret_cast<void*>(GetCurrentProcessId);
    m_apiMap["KERNEL32.DLL"]["QueryPerformanceCounter"] = reinterpret_cast<void*>(QueryPerformanceCounter);
    m_apiMap["KERNEL32.DLL"]["EncodePointer"] = reinterpret_cast<void*>(EncodePointer);
    m_apiMap["KERNEL32.DLL"]["GetModuleHandleW"] = reinterpret_cast<void*>(GetModuleHandleW);
    m_apiMap["KERNEL32.DLL"]["GetProcAddress"] = reinterpret_cast<void*>(GetProcAddress);
    m_apiMap["KERNEL32.DLL"]["InitializeCriticalSectionAndSpinCount"] = reinterpret_cast<void*>(InitializeCriticalSectionAndSpinCount);
    m_apiMap["KERNEL32.DLL"]["TlsAlloc"] = reinterpret_cast<void*>(TlsAlloc);
    m_apiMap["KERNEL32.DLL"]["TlsSetValue"] = reinterpret_cast<void*>(TlsSetValue);
    m_apiMap["KERNEL32.DLL"]["EnterCriticalSection"] = reinterpret_cast<void*>(EnterCriticalSection);
    m_apiMap["KERNEL32.DLL"]["LeaveCriticalSection"] = reinterpret_cast<void*>(LeaveCriticalSection);
    m_apiMap["KERNEL32.DLL"]["GetCommandLineA"] = reinterpret_cast<void*>(GetCommandLineA);
    m_apiMap["KERNEL32.DLL"]["GetEnvironmentStringsW"] = reinterpret_cast<void*>(GetEnvironmentStringsW);
    m_apiMap["KERNEL32.DLL"]["WideCharToMultiByte"] = reinterpret_cast<void*>(WideCharToMultiByte);
    m_apiMap["KERNEL32.DLL"]["FreeEnvironmentStringsW"] = reinterpret_cast<void*>(FreeEnvironmentStringsW);
    m_apiMap["KERNEL32.DLL"]["GetStartupInfoW"] = reinterpret_cast<void*>(GetStartupInfoW);
    m_apiMap["KERNEL32.DLL"]["GetStdHandle"] = reinterpret_cast<void*>(GetStdHandle);
    m_apiMap["KERNEL32.DLL"]["GetFileType"] = reinterpret_cast<void*>(GetFileType);
    m_apiMap["KERNEL32.DLL"]["GetLastError"] = reinterpret_cast<void*>(GetLastError);
    m_apiMap["KERNEL32.DLL"]["TlsGetValue"] = reinterpret_cast<void*>(TlsGetValue);
    m_apiMap["KERNEL32.DLL"]["SetLastError"] = reinterpret_cast<void*>(SetLastError);
    m_apiMap["KERNEL32.DLL"]["GetACP"] = reinterpret_cast<void*>(GetACP);
    m_apiMap["KERNEL32.DLL"]["GetModuleFileNameA"] = reinterpret_cast<void*>(GetModuleFileNameA);
    m_apiMap["KERNEL32.DLL"]["DeleteCriticalSection"] = reinterpret_cast<void*>(DeleteCriticalSection);
    m_apiMap["KERNEL32.DLL"]["TlsFree"] = reinterpret_cast<void*>(TlsFree);
    m_apiMap["KERNEL32.DLL"]["DecodePointer"] = reinterpret_cast<void*>(EncodePointer);
    m_apiMap["KERNEL32.DLL"]["HeapSize"] = reinterpret_cast<void*>(HeapSize);

    // Set locale for wide string conversion functions
    setlocale(LC_ALL, "en_US.utf8");
}

WindowsAPI& WindowsAPI::GetInstance() {
    static WindowsAPI instance;
    return instance;
}

void* WindowsAPI::GetFunction(const std::string& moduleName, const std::string& funcName)
{
    auto modDataIt = m_apiMap.find(moduleName);
    if (modDataIt != m_apiMap.end()) {
        auto symDataIt = modDataIt->second.find(funcName);
        if (symDataIt != modDataIt->second.end()) {
            return symDataIt->second;
        }
    }
    return nullptr;
}
