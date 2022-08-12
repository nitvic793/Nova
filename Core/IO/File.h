#pragma once

#include <streambuf>
#include <istream>

namespace nv::io
{ 
    // https://stackoverflow.com/questions/13059091/creating-an-input-stream-from-constant-memory
    struct MemoryBuffer : std::streambuf 
    {
        MemoryBuffer(char const* base, size_t size) 
        {
            char* p(const_cast<char*>(base));
            this->setg(p, p, p + size);
        }
    };

    struct MemoryStream : virtual MemoryBuffer, std::istream 
    {
        MemoryStream(char const* base, size_t size)
            : MemoryBuffer(base, size)
            , std::istream(static_cast<std::streambuf*>(this)) 
        {}
    };
}