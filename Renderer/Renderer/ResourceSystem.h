#pragma once

#include <Lib/Handle.h>
#include <Lib/Pool.h>
#include <Renderer/CommonDefines.h>
#include <AssetBase.h>
#include <Engine/System.h>

namespace nv::graphics
{
    class ResourceSystem : public ISystem
    {
    public:
        void Init() override;
        void Update(float deltaTime, float totalTime) override;
        void Destroy() override {}
        void OnReload() override {}
    private:
    };
}