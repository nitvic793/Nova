
#pragma once


#include <Engine/System.h>
#include <Store/Store.h>

namespace nv
{
    class SimDriver : public ISystem
    {
    public:
        void Init() override;
        void Update(float deltaTime, float totalTime) override;
        void Destroy() override;

    private:
       sim::Store mAgentStore;
    };
}
