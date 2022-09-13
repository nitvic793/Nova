#pragma once

#include <cstdint>
#include <string>

namespace nv::io
{
    using Byte = uint8_t;

    size_t  GetFileSize(const char* filename, bool isTextFile = false);
    bool    ReadFile(const char* filename, Byte*& pOutBuffer, uint32_t maxSize);
    void    NormalizePath(std::string& path);
}