#include "pch.h"
#include "Utility.h"
#include <fstream>

namespace nv::io
{
    size_t GetFileSize(const char* filename)
    {
        std::ifstream is(filename);
        is.seekg(0, std::ios_base::end);
        size_t size = is.tellg();
        return size;
    }

    bool LoadFile(const char* filename, Byte*& pOutBuffer, uint32_t maxSize)
    {
        pOutBuffer = nullptr;
        return true;
    }
}

