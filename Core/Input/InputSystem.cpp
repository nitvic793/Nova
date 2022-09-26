#include "pch.h"
#include "InputSystem.h"

#if NV_PLATFORM_WINDOWS
#include <DX12/WindowDX12.h>
#endif

namespace nv::input
{
    InputState* gpInputState = nullptr;

    const InputState& GetInputState()
    {
        return *gpInputState;
    }

    bool IsKeyPressed(Keys key)
    {
        return gpInputState->mKeyboard.IsKeyPressed(key);
    }

    bool IsKeyReleased(Keys key)
    {
        return gpInputState->mKeyboard.IsKeyReleased(key);
    }

    bool IsKeyDown(Keys key)
    {
        return gpInputState->mKeyboard.GetLastState().IsKeyDown(key);
    }

    ButtonState LeftMouseButtonState()
    {
        return gpInputState->mMouse.leftButton;
    }

    ButtonState RightMouseButtonState()
    {
        return gpInputState->mMouse.rightButton;
    }

    ButtonState MiddleMouseButtonState()
    {
        return gpInputState->mMouse.middleButton;
    }

    int32_t GetMouseScrollWheelValue()
    {
        return gpInputState->mMouse.GetLastState().scrollWheelValue;
    }

    void InputSystem::Init()
    {
        mKeyboard = MakeScoped<Keyboard, true>();
        mMouse = MakeScoped<Mouse, true>();

        gpInputState = Alloc<InputState>();
        gpInputState->mpKeyboardInstance = mKeyboard.Get();
        gpInputState->mpMouseInstance = mMouse.Get();

#if NV_PLATFORM_WINDOWS
        auto window = (graphics::WindowDX12*)graphics::gWindow;
        window->SetInputWindow(mKeyboard.Get(), mMouse.Get());
#endif
    }

    void InputSystem::Update(float deltaTime, float totalTime)
    {
        auto kb = mKeyboard->GetState();
        auto mouse = mMouse->GetState();

        gpInputState->mKeyboard.Update(kb);
        gpInputState->mMouse.Update(mouse);
    }

    void InputSystem::Destroy()
    {
        Free(gpInputState);
    }
}


