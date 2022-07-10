#pragma once

namespace nv::graphics
{
    class Window;

    class Device
    {
    public:
        static constexpr int kFrameBufferCount = 3;

        virtual bool Init(Window& window) = 0;
        virtual ~Device() {}
    };
}