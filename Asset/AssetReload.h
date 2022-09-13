#pragma once

#include <AssetBase.h>
#include <Engine/EventSystem.h>

namespace nv::asset
{
    struct AssetReloadEvent : public IEvent
    {
        AssetID mAssetId; 
    };
}