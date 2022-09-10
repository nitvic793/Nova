#include "pch.h"

#include <Lib/Util.h>
#include <locale>
#include <codecvt>

namespace nv 
{
    static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> sgConverter;

    std::wstring nv::ToWString(const std::string& str)
    {
        const std::wstring wide = sgConverter.from_bytes(str);
        return wide;
    }

    std::string ToString(const std::wstring& str)
    {
        const std::string out = sgConverter.to_bytes(str);
        return out;
    }
}
