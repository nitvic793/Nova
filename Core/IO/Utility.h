#pragma once

#include <cstdint>

namespace nv::io
{
    using Byte = uint8_t;

    size_t  GetFileSize(const char* filename, bool isTextFile = false);
    bool    ReadFile(const char* filename, Byte*& pOutBuffer, uint32_t maxSize);
}