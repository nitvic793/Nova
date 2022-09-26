#pragma once

#include <Keyboard.h>
#include <Mouse.h>

namespace nv::input
{
    using Keys = DirectX::Keyboard::Keys;
    using ButtonState = DirectX::Mouse::ButtonStateTracker::ButtonState;

    bool        IsKeyPressed(Keys key);
    bool        IsKeyReleased(Keys key);
    ButtonState LeftMouseButtonState();
    ButtonState RightMouseButtonState();
    ButtonState MiddleMouseButtonState();
    int32_t     GetMouseScrollWheelValue();
}