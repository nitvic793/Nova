#include "pch.h"
#include "Utility.h"
#include <fstream>

namespace nv::io
{
    size_t GetFileSize(const char* filename, bool isTextFile /*= false*/)
    {
        std::ifstream is(filename, std::ios::binary);
        if (!is.is_open())
            return 0;
        is.seekg(0, std::ios_base::end); 
        size_t size = is.tellg();
        return size;
    }

    bool ReadFile(const char* filename, Byte*& pOutBuffer, uint32_t readSize)
    {
        std::ifstream is(filename, std::ios::binary);
        is.seekg(0, std::ios_base::end);
        size_t size = is.tellg();
        is.seekg(0, std::ios_base::beg);
        is.read((char*)pOutBuffer, readSize);

        return !(is.bad() || is.fail());
    }

    void NormalizePath(std::string& path)
    {
        std::replace(path.begin(), path.end(), '\\', '/');
    }
}

