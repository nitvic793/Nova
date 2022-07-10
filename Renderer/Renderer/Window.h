#pragma once

#include <cstdint>

namespace nv::graphics
{
    class Window
    {
    public:
        enum ExecResult
        {
            kNvNone,
            kNvQuit
        };

    public:
        constexpr uint32_t  GetWidth() const { return mWidth; }
        constexpr uint32_t  GetHeight() const { return mHeight; }
        constexpr bool      IsFullScreen() const { return mbFullScreen; }

        virtual bool Init(uint32_t width, uint32_t height, bool fullscreen = false) = 0;
        virtual ExecResult ProcessMessages() { return kNvNone; }
        virtual ~Window() {}
    protected:
        uint32_t    mWidth = 0;
        uint32_t    mHeight = 0;
        bool        mbFullScreen = false;
    };

    extern Window* gWindow;
}