
#include "windows_library.h"

typedef __attribute__((ms_abi)) size_t(*tDecompressFunc)(uint8_t * srcBuf, size_t srcLen, uint8_t * dstBuf, size_t dstLen, int64_t fuzz, int64_t crc, int64_t verbose, uint8_t * dstBase, size_t e, void * cb, void * cbCtx, void * scratch, size_t scratchSize, int64_t threadPhase);

typedef __attribute__((ms_abi)) size_t(*tCompressFunc)(int64_t codec, uint8_t * srcBuf, size_t srcLen, uint8_t * dstBuf, int64_t level, void * opts, size_t offs, size_t unused, void * scratch, size_t scratchSize);

class OodleWrapper {
public:
    OodleWrapper() :
        m_oodleLib(WindowsLibrary::Load("oo2core_8_win64.dll"))
    {
        m_decompressFunc = reinterpret_cast<tDecompressFunc>(m_oodleLib.GetExport("OodleLZ_Decompress"));
        m_compressFunc = reinterpret_cast<tCompressFunc>(m_oodleLib.GetExport("OodleLZ_Compress"));
    }

    size_t Decompress(uint8_t * srcBuf, size_t srcLen, uint8_t * dstBuf, size_t dstLen, int64_t fuzz, int64_t crc, int64_t verbose, uint8_t * dstBase, size_t e, void * cb, void * cbCtx, void * scratch, size_t scratchSize, int64_t threadPhase)
    {
        WindowsLibrary::SetupCall();
        return m_decompressFunc(srcBuf, srcLen, dstBuf, dstLen, fuzz, crc, verbose, dstBase, e, cb, cbCtx, scratch, scratchSize, threadPhase);
    }
    size_t Compress(int64_t codec, uint8_t * srcBuf, size_t srcLen, uint8_t * dstBuf, int64_t level, void * opts, size_t offs, size_t unused, void * scratch, size_t scratchSize)
    {
        WindowsLibrary::SetupCall();
        return m_compressFunc(codec, srcBuf, srcLen, dstBuf, level, opts, offs, unused, scratch, scratchSize);
    }

private:
    WindowsLibrary m_oodleLib;
    tDecompressFunc m_decompressFunc;
    tCompressFunc m_compressFunc;
};

OodleWrapper g_oodleWrapper;

extern "C" size_t OodleLZ_Decompress(uint8_t * srcBuf, size_t srcLen, uint8_t * dstBuf, size_t dstLen, int64_t fuzz, int64_t crc, int64_t verbose, uint8_t * dstBase, size_t e, void * cb, void * cbCtx, void * scratch, size_t scratchSize, int64_t threadPhase)
{
    return g_oodleWrapper.Decompress(srcBuf, srcLen, dstBuf, dstLen, fuzz, crc, verbose, dstBase, e, cb, cbCtx, scratch, scratchSize, threadPhase);
}

extern "C" size_t OodleLZ_Compress(int64_t codec, uint8_t * srcBuf, size_t srcLen, uint8_t * dstBuf, int64_t level, void * opts, size_t offs, size_t unused, void * scratch, size_t scratchSize)
{
    return g_oodleWrapper.Compress(codec, srcBuf, srcLen, dstBuf, level, opts, offs, unused, scratch, scratchSize);
}
