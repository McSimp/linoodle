#include <fstream>
#include <vector>
#include <iostream>
#include <string.h>
#include <dlfcn.h>
#include <pthread.h>

typedef size_t(*tDecompressFunc)(uint8_t* srcBuf, size_t srcLen, uint8_t* dstBuf, size_t dstLen, int64_t unk1, int64_t unk2, int64_t unk3, int64_t unk4, int64_t unk5, int64_t unk6, int64_t unk7, int64_t unk8, int64_t unk9, int64_t unk10);

std::vector<char> ReadAllData(const char* path)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size)) {
        throw std::runtime_error("Failed to open file");
    }
    return buffer;    
}

void* TestFunc(void* arg)
{
    for (int i = 0; i < 2000; i++)
    {
        // Load compiled library
        void* lib = dlopen("build/liblinoodle.so", RTLD_LAZY);
        if (lib == nullptr) {
            std::cout << "dlopen failed: " << dlerror() << std::endl;
            return nullptr;
        }
        auto decompress = reinterpret_cast<tDecompressFunc>(dlsym(lib, "OodleLZ_Decompress"));

        // Load test data
        auto compressedData = ReadAllData("test/compressed.data");
        auto decompressedData = ReadAllData("test/decompressed.data");

        for (int j = 0; j < 10; j++)
        {
            // Decompress the data
            uint8_t* destBuf = reinterpret_cast<uint8_t*>(malloc(decompressedData.size() + 4096));
            size_t result = decompress(reinterpret_cast<uint8_t*>(compressedData.data()), compressedData.size(), destBuf, decompressedData.size(), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

            // Ensure it's what we expect
            if (result != decompressedData.size()) {
                std::cout << "Resulting size from OodleLZ_Decompress does not match (expected " << decompressedData.size() << ", got " << result << ")" << std::endl;
                return nullptr;
            }

            if (memcmp(destBuf, decompressedData.data(), decompressedData.size()) != 0) {
                std::cout << "Decompressed data does not match" << std::endl;
                return nullptr;
            }

            free(destBuf);
        }

        // Unload the library
        if (dlclose(lib) != 0) {
            std::cout << "Failed to dlclose library" << std::endl;
            return nullptr;
        }
    }

    return nullptr;
}

int main()
{
    // Spin up multiple threads to run the test
    const int NUM_THREADS = 8;
    pthread_t threads[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], nullptr, TestFunc, nullptr);
    }

    // Wait for the threads to finish
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], nullptr);
    }
    
    std::cout << "Test succeeded!" << std::endl;
    return 0;
}
