#ifndef NV_INPUT_SYSTEM
#define NV_INPUT_SYSTEM

#pragma once

#include <Engine/System.h>
#include <Input/InputState.h>

namespace nv::input
{
    class InputSystem : public ISystem
    {
    public:
        void Init() override;
        void Update(float deltaTime, float totalTime) override;
        void Destroy() override;
    private:

        ScopedPtr<Keyboard, true>   mKeyboard;
        ScopedPtr<Mouse, true>      mMouse;
    };
}

#endif // !NV_INPUT_SYSTEM