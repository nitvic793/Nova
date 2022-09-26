
#pragma once

#include <Engine/System.h>
#include <Input/InputState.h>

namespace nv
{
    class DriverSystem : public ISystem
    {
    public:
        void Init() override;
        void Update(float deltaTime, float totalTime) override;
        void Destroy() override;
    private:
    };
}
