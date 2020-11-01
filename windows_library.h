#pragma once

#include "windows_api.h"
#include <string>
#include <unordered_map>

class MappedMemory {
public:
    MappedMemory(void* mapping, size_t size);
    MappedMemory(const MappedMemory&) = delete;
    MappedMemory(MappedMemory&& other);
    MappedMemory& operator=(const MappedMemory&) = delete;
    MappedMemory& operator=(MappedMemory&&) = delete;
    ~MappedMemory();
    operator uint8_t* () const { return reinterpret_cast<uint8_t*>(m_mapping); }
    operator void* () const { return reinterpret_cast<void*>(m_mapping); }
    void* ptr() const { return reinterpret_cast<void*>(m_mapping); }
    size_t size() const { return m_size; }
private:
    void* m_mapping;
    size_t m_size;
};

typedef BOOL(WINAPI* tEntryPoint)(void* hinstDLL, DWORD fdwReason, void* lpReserved);

class WindowsLibrary {
public:
    WindowsLibrary(MappedMemory&& mapping, std::unordered_map<std::string, void*>&& exports, tEntryPoint entryPoint);
    WindowsLibrary(const WindowsLibrary&) = delete;
    WindowsLibrary(WindowsLibrary&& other);
    WindowsLibrary& operator=(const WindowsLibrary&) = delete;
    WindowsLibrary& operator=(WindowsLibrary&&) = delete;
    ~WindowsLibrary();
    void* GetExport(const std::string& exportName);
    void* GetBaseAddress();

    static void SetupCall();
    static WindowsLibrary Load(const char* path);
private:
    MappedMemory m_mapping;
    std::unordered_map<std::string, void*> m_exports;
    tEntryPoint m_entryPoint;
    static __thread TIB s_tib;
};
