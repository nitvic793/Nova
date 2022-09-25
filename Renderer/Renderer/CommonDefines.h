#pragma once

#include <cstdint>

namespace nv::graphics
{
    static constexpr int FRAMEBUFFER_COUNT = 3;
    
    using ResID = StringID;

    struct ConstantBufferView
    {
        uint64_t mMemoryOffset = 0;
        uint64_t mHeapIndex = 0;
    };

    enum ContextType : uint8_t
    {
        CONTEXT_GFX = 0,
        CONTEXT_COMPUTE,
        CONTEXT_UPLOAD,
        CONTEXT_RAYTRACING
    };

    enum PrimitiveTopology
    {
        PRIMITIVE_TOPOLOGY_UNDEFINED        = 0,
        PRIMITIVE_TOPOLOGY_POINTLIST        = 1,
        PRIMITIVE_TOPOLOGY_LINELIST         = 2,
        PRIMITIVE_TOPOLOGY_LINESTRIP        = 3,
        PRIMITIVE_TOPOLOGY_TRIANGLELIST     = 4,
        PRIMITIVE_TOPOLOGY_TRIANGLESTRIP    = 5,
    };

    namespace tex
    {
        enum Type : uint8_t
        {
            NONE = 0,
            BUFFER,
            TEXTURE_1D,
            TEXTURE_2D,
            TEXTURE_3D,
            TEXTURE_CUBE,
        };

        enum Usage : uint8_t
        {
            USAGE_NONE = 0,
            USAGE_SHADER = 1,
            USAGE_UNORDERED = 1 << 1,
            USAGE_RENDER_TARGET = 1 << 2,
            USAGE_DEPTH_STENCIL = 1 << 3
        };
    }

    namespace buffer
    {
        enum Flags : uint8_t
        {
            FLAG_NONE = 0,
            FLAG_ALLOW_RENDER_TARGET = 1,
            FLAG_ALLOW_DEPTH = 1 << 1,
            FLAG_ALLOW_UNORDERED = 1 << 2,
            FLAG_RAYTRACING_ACCELERATION = 1 << 3
        };

        enum BufferMode : uint8_t
        {
            BUFFER_MODE_DEFAULT = 1,
            BUFFER_MODE_UPLOAD = 2,
            BUFFER_MODE_READBACK = 3,
            BUFFER_MODE_CUSTOM = 4
        };

        enum Type : uint8_t
        {
            TYPE_NONE = 0,
            TYPE_BUFFER,
            TYPE_TEXTURE_1D,
            TYPE_TEXTURE_2D,
            TYPE_TEXTURE_3D
        };

        enum State : uint16_t
        {
            STATE_COMMON = 0,
            STATE_INDEX_BUFFER,
            STATE_VERTEX_BUFFER,
            STATE_STORAGE_BUFFER,
            STATE_UNORDERED_ACCESS,
            STATE_DEPTH_WRITE,
            STATE_DEPTH_READ,
            STATE_PIXEL_SHADER_RESOURCE,
            STATE_SHADER_RESOURCE,
            STATE_COPY_DEST,
            STATE_COPY_SOURCE,
            STATE_INDIRECT_ARG,
            STATE_RENDER_TARGET,
            STATE_PRESENT,
            STATE_GENERIC_READ
        };
    }

    struct Rect
    {
        int32_t mLeft;
        int32_t mTop;
        int32_t mRight;
        int32_t mBottom;
    };

    struct Viewport
    {
        float mTopLeftX;
        float mTopLeftY;
        float mWidth;
        float mHeight;
        float mMinDepth;
        float mMaxDepth;
    };
}