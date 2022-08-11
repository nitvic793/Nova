#include "pch.h"
#include "Utility.h"
#include <fstream>

namespace nv::io
{
    size_t GetFileSize(const char* filename)
    {
        std::ifstream is(filename, std::ios::binary);
        is.seekg(0, std::ios_base::end);
        size_t size = is.tellg();
        return size;
    }

    bool ReadFile(const char* filename, Byte*& pOutBuffer, uint32_t readSize)
    {
        pOutBuffer = nullptr;
        std::ifstream is(filename, std::ios::binary);
        is.seekg(0, std::ios_base::end);
        size_t size = is.tellg();
        is.seekg(0, std::ios_base::beg);
        is.read((char*)pOutBuffer, readSize);

        return !(is.bad() || is.fail());
    }
}

