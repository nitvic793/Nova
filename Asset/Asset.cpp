
#include "pch.h"
#include "framework.h"

#include <Asset.h>


namespace nv::asset
{
    constexpr auto HEADER_SIZE = sizeof(Header);

    void Serialize(const Array<Asset>& assets, Array<uint8_t>& outBuffer, IAllocator* pAllocator)
    {

    }

    void Deserialize(const Array<uint8_t>& buffer, Array<Asset>& outAssets, IAllocator* pAllocator)
    {
    }
}
