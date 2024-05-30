#include "pch.h"

#include "StringHash.h"

namespace nv
{
    void StringDB::AddString(const char* str, StringID id)
    {
        mStringDB[id] = str;
    }

    void StringDB::AddString(const std::string& str, StringID id)
    {
        mStringDB[id] = str;
    }

    const std::string& StringDB::GetString(StringID id) const
    {
        return mStringDB.at(id);
    }

    StringDB& StringDB::Get()
    {
        static StringDB sStringDB;
        return sStringDB;
    }
}