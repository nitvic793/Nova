#pragma once

#include <Keyboard.h>
#include <Mouse.h>

namespace nv::input
{
    using Keyboard = DirectX::Keyboard;
    using Mouse = DirectX::Mouse;
    using KeyboardState = Keyboard::State;
    using KBStateTracker = Keyboard::KeyboardStateTracker;
    using MouseStateTracker = Mouse::ButtonStateTracker;
    using Keys = Keyboard::Keys;
    using ButtonState = MouseStateTracker::ButtonState;

    struct InputState
    {
        KBStateTracker      mKeyboard;
        MouseStateTracker   mMouse;
        Keyboard*           mpKeyboardInstance;
        Mouse*              mpMouseInstance;
    };

    extern InputState* gpInputState;

    const InputState& GetInputState();
}