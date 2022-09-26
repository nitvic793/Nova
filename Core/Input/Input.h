#pragma once

#include <Keyboard.h>
#include <Mouse.h>
#include <concepts>
#include <array>

namespace nv::input
{
    using Keys = DirectX::Keyboard::Keys;
    using ButtonState = DirectX::Mouse::ButtonStateTracker::ButtonState;

    bool        IsKeyPressed(Keys key);
    bool        IsKeyReleased(Keys key);
    bool        IsKeyDown(Keys key);

    ButtonState LeftMouseButtonState();
    ButtonState RightMouseButtonState();
    ButtonState MiddleMouseButtonState();
    int32_t     GetMouseScrollWheelValue();


    bool IsKeyComboPressed(std::convertible_to<Keys> auto ...keys)
    {
        std::array<Keys, sizeof...(keys)> keyList = { { keys ... } };
        bool result = true;
        uint32_t i = 0;
        for (const Keys& key : keyList)
        {
            if (i == keyList.size() - 1)
            {
                result = result && IsKeyPressed(key);
                continue;
            }

            result = result && IsKeyDown(key);
            i++;
        }

        return result;
    }
}