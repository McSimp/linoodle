#include "windows_library.h"
#include "windows_api.h"
#include <asm/prctl.h>
#include <asm/unistd_64.h>
#include <cstring>
#include <memory>
#include <pe-parse/parse.h>
#include <sys/mman.h>
#include <unistd.h>

MappedMemory::MappedMemory(void* mapping, size_t size) :
    m_mapping(mapping),
    m_size(size)
{

}

MappedMemory::MappedMemory(MappedMemory&& other) :
    m_mapping(std::exchange(other.m_mapping, nullptr)),
    m_size(std::exchange(other.m_size, 0))
{

}

MappedMemory::~MappedMemory()
{
    if (m_mapping != nullptr) {
        munmap(m_mapping, m_size);
    }
}

__thread TIB WindowsLibrary::s_tib;

WindowsLibrary::WindowsLibrary(MappedMemory&& mapping, std::unordered_map<std::string, void*>&& exports, tEntryPoint entryPoint) :
    m_mapping(std::move(mapping)),
    m_exports(std::move(exports)),
    m_entryPoint(entryPoint)
{

}

WindowsLibrary::WindowsLibrary(WindowsLibrary&& other) :
    m_mapping(std::move(other.m_mapping)),
    m_exports(std::move(other.m_exports))
{

}

WindowsLibrary::~WindowsLibrary()
{
    SetupCall();
    m_entryPoint(m_mapping, DLL_PROCESS_DETACH, nullptr);
}

void* WindowsLibrary::GetExport(const std::string& exportName)
{
    return m_exports[exportName];
}

void* WindowsLibrary::GetBaseAddress()
{
    return m_mapping;
}

void WindowsLibrary::SetupCall()
{
    // Setup gs to point to a fake TIB structure
    memset(&s_tib, 0, sizeof(TIB));
    s_tib.Self = &s_tib;
    syscall(__NR_arch_prctl, ARCH_SET_GS, (void*)&s_tib);
}

using ParsedPeRef = std::unique_ptr<peparse::parsed_pe, void (*)(peparse::parsed_pe*)>;

void RelocateImage(const MappedMemory& imageMapping, const ParsedPeRef& pe)
{
    uint64_t delta = reinterpret_cast<uint64_t>(imageMapping.ptr()) - pe->peHeader.nt.OptionalHeader64.ImageBase;
    if (delta == 0) {
        return;
    }

    // Check that the PE is relocatable
    if (!(pe->peHeader.nt.OptionalHeader64.DllCharacteristics & peparse::IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE)) {
        throw std::runtime_error("PE is not relocatable");
    }

    // Perform relocations
    struct RelocContext {
        uint64_t delta;
        const MappedMemory& imageMapping;
    };
    RelocContext ctx{ delta, imageMapping };
    auto relocIt = [](void* pRawCtx, const peparse::VA& relocAddr, const peparse::reloc_type& type) -> int {
        RelocContext* pCtx = static_cast<RelocContext*>(pRawCtx);
        if (type == peparse::RELOC_ABSOLUTE) {
            return 0;
        }
        else if (type == peparse::RELOC_DIR64) {
            uint64_t* pData = reinterpret_cast<uint64_t*>(relocAddr + pCtx->delta);
            *pData += pCtx->delta;
            return 0;
        }
        else {
            throw std::runtime_error("Unhandled relocation type");
        }
    };

    peparse::IterRelocs(pe.get(), relocIt, &ctx);
}

WindowsLibrary WindowsLibrary::Load(const char* path)
{
    // Parse the PE
    ParsedPeRef pe(peparse::ParsePEFromFile(path), peparse::DestructParsedPE);
    if (!pe) {
        throw std::runtime_error("Failed to parse PE");
    }

    // Ensure it's an x64 image
    if (pe->peHeader.nt.FileHeader.Machine != peparse::IMAGE_FILE_MACHINE_AMD64) {
        throw std::runtime_error("PE is not an AMD64 image");
    }

    // Allocate memory for the image
    void* rawImageMapping = mmap(nullptr, pe->peHeader.nt.OptionalHeader64.SizeOfImage, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (rawImageMapping == MAP_FAILED) {
        throw std::runtime_error("Failed to allocate memory for image");
    }
    MappedMemory imageMapping(rawImageMapping, pe->peHeader.nt.OptionalHeader64.SizeOfImage);

    // Copy headers and make them read only
    uint32_t headerSize = pe->peHeader.nt.OptionalHeader64.SizeOfHeaders;
    if (headerSize > imageMapping.size()) {
        throw std::runtime_error("Invalid header size");
    }
    memcpy(imageMapping, pe->fileBuffer->buf, headerSize);
    if (mprotect(imageMapping, headerSize, PROT_READ) == -1) {
        throw std::runtime_error("Failed to mark header in image as read only");
    }

    // Copy all the sections
    auto sectionCopyIt = [](void* imageMapping, const peparse::VA& secBase, const std::string& secName, const peparse::image_section_header& s, const peparse::bounded_buffer* data) -> int {
        memcpy(static_cast<uint8_t*>(imageMapping) + s.VirtualAddress, data->buf, data->bufLen); // TODO: Potential for memory corruption here
        return 0;
    };
    peparse::IterSec(pe.get(), sectionCopyIt, imageMapping);

    // Relocate all the sections
    RelocateImage(imageMapping, pe);

    // Resolve imports we care about
    uint64_t delta = reinterpret_cast<uint64_t>(imageMapping.ptr()) - pe->peHeader.nt.OptionalHeader64.ImageBase;
    auto importIt = [](void* pDelta, const peparse::VA& impAddr, const std::string& modName, const std::string& symName) -> int {
        void* func = WindowsAPI::GetInstance().GetFunction(modName, symName);
        uint64_t delta = *reinterpret_cast<uint64_t*>(pDelta);
        void** pData = reinterpret_cast<void**>(impAddr + delta);
        if (func != nullptr) {
            *pData = func;
        }
        return 0;
    };
    peparse::IterImpVAString(pe.get(), importIt, &delta);

    // Set correct permissions on sections
    auto sectionPermIt = [](void* imageMapping, const peparse::VA& secBase, const std::string& secName, const peparse::image_section_header& s, const peparse::bounded_buffer* data) -> int {
        int prot = 0;
        if (s.Characteristics & peparse::IMAGE_SCN_MEM_EXECUTE)
            prot |= PROT_EXEC;
        if (s.Characteristics & peparse::IMAGE_SCN_MEM_WRITE)
            prot |= PROT_WRITE;
        if (s.Characteristics & peparse::IMAGE_SCN_MEM_READ)
            prot |= PROT_READ;
        if (mprotect(static_cast<uint8_t*>(imageMapping) + s.VirtualAddress, s.Misc.VirtualSize, prot) == -1) {
            throw std::runtime_error("Failed to mprotect section");
        }
        return 0;
    };
    peparse::IterSec(pe.get(), sectionPermIt, imageMapping);

    // TODO: Exceptions
    // TODO: Security cookie
    // TODO: Execute TLS callbacks

    // Get addresses of exports
    struct ExportsContext {
        uint64_t delta;
        std::unordered_map<std::string, void*> exports;
    };
    ExportsContext ctx{ delta };
    auto exportIt = [](void* rawCtx, const peparse::VA& funcAddr, const std::string& mod, const std::string& func) -> int {
        auto pCtx = static_cast<ExportsContext*>(rawCtx);
        pCtx->exports.emplace(func, reinterpret_cast<void*>(funcAddr + pCtx->delta));
        return 0;
    };
    peparse::IterExpVA(pe.get(), exportIt, &ctx);

    // Execute the entrypoint
    SetupCall();
    tEntryPoint entryPoint = reinterpret_cast<tEntryPoint>(imageMapping + pe->peHeader.nt.OptionalHeader64.AddressOfEntryPoint);
    BOOL result = entryPoint(imageMapping, DLL_PROCESS_ATTACH, nullptr);
    if (!result) {
        throw std::runtime_error("Library entry point returned FALSE");
    }

    return WindowsLibrary(std::move(imageMapping), std::move(ctx.exports), entryPoint);
}
