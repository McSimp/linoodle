
#include "windows_library.h"

typedef __attribute__((ms_abi)) size_t(*tDecompressFunc)(uint8_t* srcBuf, size_t srcLen, uint8_t* dstBuf, size_t dstLen, int64_t unk1, int64_t unk2, int64_t unk3, int64_t unk4, int64_t unk5, int64_t unk6, int64_t unk7, int64_t unk8, int64_t unk9, int64_t unk10);

class OodleWrapper {
public:
    OodleWrapper() :
        m_oodleLib(WindowsLibrary::Load("../oo2core_8_win64.dll"))
    {
        m_decompressFunc = reinterpret_cast<tDecompressFunc>(m_oodleLib.GetExport("OodleLZ_Decompress"));
    }

    size_t Decompress(uint8_t* srcBuf, size_t srcLen, uint8_t* dstBuf, size_t dstLen, int64_t unk1, int64_t unk2, int64_t unk3, int64_t unk4, int64_t unk5, int64_t unk6, int64_t unk7, int64_t unk8, int64_t unk9, int64_t unk10)
    {
        WindowsLibrary::SetupCall();
        return m_decompressFunc(srcBuf, srcLen, dstBuf, dstLen, unk1, unk2, unk3, unk4, unk5, unk6, unk7, unk8, unk9, unk10);
    }

private:
    WindowsLibrary m_oodleLib;
    tDecompressFunc m_decompressFunc;
};

OodleWrapper g_oodleWrapper;
extern "C" size_t OodleLZ_Decompress(uint8_t * srcBuf, size_t srcLen, uint8_t * dstBuf, size_t dstLen, int64_t unk1, int64_t unk2, int64_t unk3, int64_t unk4, int64_t unk5, int64_t unk6, int64_t unk7, int64_t unk8, int64_t unk9, int64_t unk10)
{
    return g_oodleWrapper.Decompress(srcBuf, srcLen, dstBuf, dstLen, unk1, unk2, unk3, unk4, unk5, unk6, unk7, unk8, unk9, unk10);
}
