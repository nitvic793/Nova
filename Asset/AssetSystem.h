#ifndef NV_ASSET_SYSTEM
#define NV_ASSET_SYSTEM

#pragma once

#include <Engine/System.h>

namespace nv::asset
{
    class AssetSystem : public ISystem
    {
    public:
        void Init() override;
        void Update(float deltaTime, float totalTime) override;
        void Destroy() override;
        void OnReload() override;
    private:

    };
}

#endif // !NV_ASSET_SYSTEM
