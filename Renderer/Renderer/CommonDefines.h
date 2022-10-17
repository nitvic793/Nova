#pragma once

#include <cstdint>

namespace nv::graphics
{
    static constexpr int FRAMEBUFFER_COUNT = 3;
    
    using ResID = StringID;

    constexpr ResID RES_ID_NULL = 0;

    struct ResourceBase
    {
        template<typename T>
        constexpr T* As() { return (T*)this; }
    };

    struct ConstantBufferView
    {
        static constexpr uint64_t INVALID_OFFSET = UINT64_MAX;

        uint64_t mMemoryOffset = INVALID_OFFSET;
        uint64_t mHeapIndex = INVALID_OFFSET;

        constexpr bool IsValid() const { return (mMemoryOffset != INVALID_OFFSET) && (mHeapIndex != INVALID_OFFSET); }
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

    enum PrimitiveTopologyType
    {
        PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED   = 0,
        PRIMITIVE_TOPOLOGY_TYPE_POINT       = 1,
        PRIMITIVE_TOPOLOGY_TYPE_LINE        = 2,
        PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE    = 3,
        PRIMITIVE_TOPOLOGY_TYPE_PATCH       = 4
    };

    constexpr uint32_t MAX_RENDER_TARGET_COUNT = 8;

    enum DepthWriteMask
    {
        DEPTH_WRITE_MASK_ZERO   = 0,
        DEPTH_WRITE_MASK_ALL    = 1
    };

    enum ComparisonFunc
    {
        COMPARISON_FUNC_NONE            = 0,
        COMPARISON_FUNC_NEVER           = 1,
        COMPARISON_FUNC_LESS            = 2,
        COMPARISON_FUNC_EQUAL           = 3,
        COMPARISON_FUNC_LESS_EQUAL      = 4,
        COMPARISON_FUNC_GREATER         = 5,
        COMPARISON_FUNC_NOT_EQUAL       = 6,
        COMPARISON_FUNC_GREATER_EQUAL   = 7,
        COMPARISON_FUNC_ALWAYS          = 8
    };

    enum FillMode
    {
        FILL_MODE_WIREFRAME = 2,
        FILL_MODE_SOLID     = 3
    };

    enum CullMode
    {
        CULL_MODE_NONE  = 1,
        CULL_MODE_FRONT = 2,
        CULL_MODE_BACK  = 3
    };

    enum ConservativeRasterMode
    {
        CONSERVATIVE_RASTERIZATION_MODE_OFF = 0,
        CONSERVATIVE_RASTERIZATION_MODE_ON  = 1
    };

    struct DepthStencilState
    {
        bool            mDepthEnable = true;
        DepthWriteMask  mDepthWriteMask = DEPTH_WRITE_MASK_ALL;
        ComparisonFunc  mDepthFunc = COMPARISON_FUNC_LESS;
        // TODO: Finish support for Stencil
        const bool      mStencilEnable = false;
    };

    struct RasterizerState
    {
        FillMode    mFillMode = FILL_MODE_SOLID;
        CullMode    mCullMode = CULL_MODE_BACK;
        bool        mFrontCounterClockwise = false;
        int32_t     mDepthBias = 0;
        float       mDepthBiasClamp = 0.f;
        float       mSlopeScaledDepthBias = 0.f;
        bool        mDepthClipEnable = true;
        bool        mMultisampleEnable = false;
        bool        mAntialiasedLineEnable = false;
        uint32_t    mForcedSampleCount = 0;
        ConservativeRasterMode mConservativeRasterMode = CONSERVATIVE_RASTERIZATION_MODE_OFF;
    };

    enum Blend
    {
        BLEND_ZERO              = 1,
        BLEND_ONE               = 2,
        BLEND_SRC_COLOR         = 3,
        BLEND_INV_SRC_COLOR     = 4,
        BLEND_SRC_ALPHA         = 5,
        BLEND_INV_SRC_ALPHA     = 6,
        BLEND_DEST_ALPHA        = 7,
        BLEND_INV_DEST_ALPHA    = 8,
        BLEND_DEST_COLOR        = 9,
        BLEND_INV_DEST_COLOR    = 10,
        BLEND_SRC_ALPHA_SAT     = 11,
        BLEND_BLEND_FACTOR      = 14,
        BLEND_INV_BLEND_FACTOR  = 15,
        BLEND_SRC1_COLOR        = 16,
        BLEND_INV_SRC1_COLOR    = 17,
        BLEND_SRC1_ALPHA        = 18,
        BLEND_INV_SRC1_ALPHA    = 19,
        BLEND_ALPHA_FACTOR      = 20,
        BLEND_INV_ALPHA_FACTOR  = 21
    };

    enum LogicOp
    {
        LOGIC_OP_CLEAR          = 0,
        LOGIC_OP_SET            = (LOGIC_OP_CLEAR + 1),
        LOGIC_OP_COPY           = (LOGIC_OP_SET + 1),
        LOGIC_OP_COPY_INVERTED  = (LOGIC_OP_COPY + 1),
        LOGIC_OP_NOOP           = (LOGIC_OP_COPY_INVERTED + 1),
        LOGIC_OP_INVERT         = (LOGIC_OP_NOOP + 1),
        LOGIC_OP_AND            = (LOGIC_OP_INVERT + 1),
        LOGIC_OP_NAND           = (LOGIC_OP_AND + 1),
        LOGIC_OP_OR             = (LOGIC_OP_NAND + 1),
        LOGIC_OP_NOR            = (LOGIC_OP_OR + 1),
        LOGIC_OP_XOR            = (LOGIC_OP_NOR + 1),
        LOGIC_OP_EQUIV          = (LOGIC_OP_XOR + 1),
        LOGIC_OP_AND_REVERSE    = (LOGIC_OP_EQUIV + 1),
        LOGIC_OP_AND_INVERTED   = (LOGIC_OP_AND_REVERSE + 1),
        LOGIC_OP_OR_REVERSE     = (LOGIC_OP_AND_INVERTED + 1),
        LOGIC_OP_OR_INVERTED    = (LOGIC_OP_OR_REVERSE + 1)
    };

    enum BlendOp
    {
        BLEND_OP_ADD            = 1,
        BLEND_OP_SUBTRACT       = 2,
        BLEND_OP_REV_SUBTRACT   = 3,
        BLEND_OP_MIN            = 4,
        BLEND_OP_MAX            = 5
    };

    enum ColorWriteEnable : uint8_t
    {
        COLOR_WRITE_ENABLE_RED      = 1,
        COLOR_WRITE_ENABLE_GREEN    = 2,
        COLOR_WRITE_ENABLE_BLUE     = 4,
        COLOR_WRITE_ENABLE_ALPHA    = 8,
        COLOR_WRITE_ENABLE_ALL      = (((COLOR_WRITE_ENABLE_RED | COLOR_WRITE_ENABLE_GREEN) | COLOR_WRITE_ENABLE_BLUE) | COLOR_WRITE_ENABLE_ALPHA)
    };

    struct RenderTargetBlendDesc
    {
        bool    mBlendEnable;
        bool    mLogicOpEnable;
        Blend   mSrcBlend;
        Blend   mDestBlend;
        BlendOp mBlendOp;
        Blend   mSrcBlendAlpha;
        Blend   mDestBlendAlpha;
        BlendOp mBlendOpAlpha;
        LogicOp mLogicOp;
        uint8_t mRenderTargetWriteMask;
    };

    struct BlendState
    {
        static constexpr RenderTargetBlendDesc DefaultBlendDesc =
        {
            false,false,
            BLEND_ONE, BLEND_ZERO, BLEND_OP_ADD,
            BLEND_ONE, BLEND_ZERO, BLEND_OP_ADD,
            LOGIC_OP_NOOP,
            COLOR_WRITE_ENABLE_ALL,
        };

        bool mAlphaToCoverageEnable = false;
        bool mIndependentBlendEnable = false;
        RenderTargetBlendDesc mRenderTarget[MAX_RENDER_TARGET_COUNT] =
        {
            DefaultBlendDesc, DefaultBlendDesc, DefaultBlendDesc, DefaultBlendDesc,
            DefaultBlendDesc, DefaultBlendDesc, DefaultBlendDesc, DefaultBlendDesc
        };
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
            RT_ACCELERATION_STRUCT
        };

        enum Usage : uint8_t
        {
            USAGE_NONE = 0,
            USAGE_SHADER = 1,
            USAGE_UNORDERED = 1 << 1,
            USAGE_RENDER_TARGET = 1 << 2,
            USAGE_DEPTH_STENCIL = 1 << 3,
            USAGE_RT_ACCELERATION = 1 << 4
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
            STATE_GENERIC_READ,
            STATE_RAYTRACING_STRUCTURE
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