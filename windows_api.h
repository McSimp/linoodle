#pragma once

#include <stdint.h>
#include <string>
#include <unordered_map>

// Tell the compiler to compile functions with WINAPI with the Windows ABI
#define WINAPI __attribute__((ms_abi))

typedef int32_t BOOL;
typedef uint32_t DWORD;
#define DLL_PROCESS_DETACH 0
#define DLL_PROCESS_ATTACH 1

typedef struct _TIB {
    void* ExceptionList;
    void* StackBase;
    void* StackLimit;
    void* SubSystemTib;
    void* FiberData;
    void* ArbitraryUserPointer;
    void* Self;
} TIB;

class WindowsAPI {
public:
    static WindowsAPI& GetInstance();
    void* GetFunction(const std::string& moduleName, const std::string& funcName);

private:
    WindowsAPI();
    ~WindowsAPI() = default;
    WindowsAPI(const WindowsAPI&) = delete;
    WindowsAPI(WindowsAPI&&) = delete;
    WindowsAPI& operator=(const WindowsAPI&) = delete;
    WindowsAPI& operator=(WindowsAPI&&) = delete;

    std::unordered_map<std::string, std::unordered_map<std::string, void*>> m_apiMap;
};
