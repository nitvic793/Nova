#pragma once

#include <NovaConfig.h>
#if NV_RENDERER_DX12

#include <Renderer/CommonDefines.h>
#include <Renderer/Format.h>
#include <d3d12.h>
#include <dxgiformat.h>

namespace nv::graphics
{
    constexpr D3D12_RESOURCE_STATES GetState(buffer::State state)
    {
        switch (state)
        {
        case buffer::STATE_COMMON:                  return D3D12_RESOURCE_STATE_COMMON;
        case buffer::STATE_INDEX_BUFFER:            return D3D12_RESOURCE_STATE_INDEX_BUFFER;
        case buffer::STATE_VERTEX_BUFFER:           return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        case buffer::STATE_STORAGE_BUFFER:          return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        case buffer::STATE_UNORDERED_ACCESS:        return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        case buffer::STATE_DEPTH_WRITE:             return D3D12_RESOURCE_STATE_DEPTH_WRITE;
        case buffer::STATE_DEPTH_READ:              return D3D12_RESOURCE_STATE_DEPTH_READ;
        case buffer::STATE_PIXEL_SHADER_RESOURCE:   return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        case buffer::STATE_SHADER_RESOURCE:         return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        case buffer::STATE_COPY_DEST:               return D3D12_RESOURCE_STATE_COPY_DEST;
        case buffer::STATE_COPY_SOURCE:             return D3D12_RESOURCE_STATE_COPY_SOURCE;
        case buffer::STATE_INDIRECT_ARG:            return D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
        case buffer::STATE_RENDER_TARGET:           return D3D12_RESOURCE_STATE_RENDER_TARGET;
        case buffer::STATE_PRESENT:                 return D3D12_RESOURCE_STATE_PRESENT;
        case buffer::STATE_GENERIC_READ:            return D3D12_RESOURCE_STATE_GENERIC_READ;
        }

        return D3D12_RESOURCE_STATE_COMMON;
    }

    constexpr D3D12_UAV_DIMENSION GetUAVDimension(tex::Type type)
    {
        switch (type)
        {
        case tex::TEXTURE_1D:   return D3D12_UAV_DIMENSION_TEXTURE1D;
        case tex::TEXTURE_2D:   return D3D12_UAV_DIMENSION_TEXTURE2D;
        case tex::TEXTURE_3D:   return D3D12_UAV_DIMENSION_TEXTURE3D;
        case tex::BUFFER:       return D3D12_UAV_DIMENSION_BUFFER;
        }

        return D3D12_UAV_DIMENSION_UNKNOWN;
    }

    constexpr D3D_PRIMITIVE_TOPOLOGY GetPrimitiveTopology(PrimitiveTopology topology)
    {
        switch (topology)
        {
        case PRIMITIVE_TOPOLOGY_UNDEFINED     : return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED    ;
        case PRIMITIVE_TOPOLOGY_POINTLIST     : return D3D_PRIMITIVE_TOPOLOGY_POINTLIST    ;
        case PRIMITIVE_TOPOLOGY_LINELIST      : return D3D_PRIMITIVE_TOPOLOGY_LINELIST     ;
        case PRIMITIVE_TOPOLOGY_LINESTRIP     : return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP    ;
        case PRIMITIVE_TOPOLOGY_TRIANGLELIST  : return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST ;
        case PRIMITIVE_TOPOLOGY_TRIANGLESTRIP : return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
        }

        return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    }

    constexpr D3D12_PRIMITIVE_TOPOLOGY_TYPE GetPrimitiveTopologyType(PrimitiveTopologyType topology)
    {
        switch (topology)
        {
            case PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED  : return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;    
            case PRIMITIVE_TOPOLOGY_TYPE_POINT      : return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT    ;    
            case PRIMITIVE_TOPOLOGY_TYPE_LINE       : return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE     ;    
            case PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE   : return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE ;    
            case PRIMITIVE_TOPOLOGY_TYPE_PATCH      : return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH    ;    
        }

        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
    }

    constexpr D3D12_DEPTH_WRITE_MASK GetDepthWriteMask(DepthWriteMask mask)
    {
        switch (mask)
        {
            case DEPTH_WRITE_MASK_ZERO  : return D3D12_DEPTH_WRITE_MASK_ZERO;
            case DEPTH_WRITE_MASK_ALL   : return D3D12_DEPTH_WRITE_MASK_ALL;
        }
    }

    constexpr D3D12_COMPARISON_FUNC GetComparisonFunc(ComparisonFunc func)
    {
        switch (func)
        {
            case COMPARISON_FUNC_NONE         : D3D12_COMPARISON_FUNC_NONE         ;
            case COMPARISON_FUNC_NEVER        : D3D12_COMPARISON_FUNC_NEVER        ;
            case COMPARISON_FUNC_LESS         : D3D12_COMPARISON_FUNC_LESS         ;
            case COMPARISON_FUNC_EQUAL        : D3D12_COMPARISON_FUNC_EQUAL        ;
            case COMPARISON_FUNC_LESS_EQUAL   : D3D12_COMPARISON_FUNC_LESS_EQUAL   ;
            case COMPARISON_FUNC_GREATER      : D3D12_COMPARISON_FUNC_GREATER      ;
            case COMPARISON_FUNC_NOT_EQUAL    : D3D12_COMPARISON_FUNC_NOT_EQUAL    ;
            case COMPARISON_FUNC_GREATER_EQUAL: D3D12_COMPARISON_FUNC_GREATER_EQUAL;
            case COMPARISON_FUNC_ALWAYS       : D3D12_COMPARISON_FUNC_ALWAYS       ;
        }
    }

    static const D3D12_RESOURCE_FLAGS GetFlags(buffer::Flags bufferFlags)
    {
        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
        flags |= (bufferFlags & buffer::FLAG_ALLOW_RENDER_TARGET)       ? D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET               : D3D12_RESOURCE_FLAG_NONE;
        flags |= (bufferFlags & buffer::FLAG_ALLOW_DEPTH)               ? D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL               : D3D12_RESOURCE_FLAG_NONE;
        flags |= (bufferFlags & buffer::FLAG_ALLOW_UNORDERED)           ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS            : D3D12_RESOURCE_FLAG_NONE;
        flags |= (bufferFlags & buffer::FLAG_RAYTRACING_ACCELERATION)   ? D3D12_RESOURCE_FLAG_RAYTRACING_ACCELERATION_STRUCTURE : D3D12_RESOURCE_FLAG_NONE;

        return flags;
    }

    constexpr D3D12_HEAP_TYPE GetHeapType(buffer::BufferMode mode)
    {
        switch (mode)
        {
        case buffer::BUFFER_MODE_DEFAULT:   return D3D12_HEAP_TYPE_DEFAULT;
        case buffer::BUFFER_MODE_UPLOAD:    return D3D12_HEAP_TYPE_UPLOAD;
        case buffer::BUFFER_MODE_READBACK:  return D3D12_HEAP_TYPE_READBACK;
        case buffer::BUFFER_MODE_CUSTOM:    return D3D12_HEAP_TYPE_CUSTOM;
        }

        return D3D12_HEAP_TYPE_DEFAULT;
    }

    constexpr D3D12_COMMAND_LIST_TYPE GetCommandListType(ContextType context)
    {
        switch (context)
        {
        case CONTEXT_COMPUTE:       return D3D12_COMMAND_LIST_TYPE_COMPUTE;
        case CONTEXT_GFX:           return D3D12_COMMAND_LIST_TYPE_DIRECT;
        case CONTEXT_UPLOAD:        return D3D12_COMMAND_LIST_TYPE_COPY;
        case CONTEXT_RAYTRACING:    return D3D12_COMMAND_LIST_TYPE_DIRECT;
        }

        return D3D12_COMMAND_LIST_TYPE_DIRECT;
    }

    // At the moment, the DXGI format directly maps to format::SurfaceFormat
    constexpr DXGI_FORMAT GetFormat(format::SurfaceFormat format)
    {
        using namespace format;
        switch (format)
        {
            case UNKNOWN	                :   return DXGI_FORMAT_UNKNOWN	                 ;
            case R32G32B32A32_TYPELESS      :   return DXGI_FORMAT_R32G32B32A32_TYPELESS     ;
            case R32G32B32A32_FLOAT         :   return DXGI_FORMAT_R32G32B32A32_FLOAT        ;
            case R32G32B32A32_UINT          :   return DXGI_FORMAT_R32G32B32A32_UINT         ;
            case R32G32B32A32_SINT          :   return DXGI_FORMAT_R32G32B32A32_SINT         ;
            case R32G32B32_TYPELESS         :   return DXGI_FORMAT_R32G32B32_TYPELESS        ;
            case R32G32B32_FLOAT            :   return DXGI_FORMAT_R32G32B32_FLOAT           ;
            case R32G32B32_UINT             :   return DXGI_FORMAT_R32G32B32_UINT            ;
            case R32G32B32_SINT             :   return DXGI_FORMAT_R32G32B32_SINT            ;
            case R16G16B16A16_TYPELESS      :   return DXGI_FORMAT_R16G16B16A16_TYPELESS     ;
            case R16G16B16A16_FLOAT         :   return DXGI_FORMAT_R16G16B16A16_FLOAT        ;
            case R16G16B16A16_UNORM         :   return DXGI_FORMAT_R16G16B16A16_UNORM        ;
            case R16G16B16A16_UINT          :   return DXGI_FORMAT_R16G16B16A16_UINT         ;
            case R16G16B16A16_SNORM         :   return DXGI_FORMAT_R16G16B16A16_SNORM        ;
            case R16G16B16A16_SINT          :   return DXGI_FORMAT_R16G16B16A16_SINT         ;
            case R32G32_TYPELESS            :   return DXGI_FORMAT_R32G32_TYPELESS           ;
            case R32G32_FLOAT               :   return DXGI_FORMAT_R32G32_FLOAT              ;
            case R32G32_UINT                :   return DXGI_FORMAT_R32G32_UINT               ;
            case R32G32_SINT                :   return DXGI_FORMAT_R32G32_SINT               ;
            case R32G8X24_TYPELESS          :   return DXGI_FORMAT_R32G8X24_TYPELESS         ;
            case D32_FLOAT_S8X24_UINT       :   return DXGI_FORMAT_D32_FLOAT_S8X24_UINT      ;
            case R32_FLOAT_X8X24_TYPELESS   :   return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS  ;
            case X32_TYPELESS_G8X24_UINT    :   return DXGI_FORMAT_X32_TYPELESS_G8X24_UINT   ;
            case R10G10B10A2_TYPELESS       :   return DXGI_FORMAT_R10G10B10A2_TYPELESS      ;
            case R10G10B10A2_UNORM          :   return DXGI_FORMAT_R10G10B10A2_UNORM         ;
            case R10G10B10A2_UINT           :   return DXGI_FORMAT_R10G10B10A2_UINT          ;
            case R11G11B10_FLOAT            :   return DXGI_FORMAT_R11G11B10_FLOAT           ;
            case R8G8B8A8_TYPELESS          :   return DXGI_FORMAT_R8G8B8A8_TYPELESS         ;
            case R8G8B8A8_UNORM             :   return DXGI_FORMAT_R8G8B8A8_UNORM            ;
            case R8G8B8A8_UNORM_SRGB        :   return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB       ;
            case R8G8B8A8_UINT              :   return DXGI_FORMAT_R8G8B8A8_UINT             ;
            case R8G8B8A8_SNORM             :   return DXGI_FORMAT_R8G8B8A8_SNORM            ;
            case R8G8B8A8_SINT              :   return DXGI_FORMAT_R8G8B8A8_SINT             ;
            case R16G16_TYPELESS            :   return DXGI_FORMAT_R16G16_TYPELESS           ;
            case R16G16_FLOAT               :   return DXGI_FORMAT_R16G16_FLOAT              ;
            case R16G16_UNORM               :   return DXGI_FORMAT_R16G16_UNORM              ;
            case R16G16_UINT                :   return DXGI_FORMAT_R16G16_UINT               ;
            case R16G16_SNORM               :   return DXGI_FORMAT_R16G16_SNORM              ;
            case R16G16_SINT                :   return DXGI_FORMAT_R16G16_SINT               ;
            case R32_TYPELESS               :   return DXGI_FORMAT_R32_TYPELESS              ;
            case D32_FLOAT                  :   return DXGI_FORMAT_D32_FLOAT                 ;
            case R32_FLOAT                  :   return DXGI_FORMAT_R32_FLOAT                 ;
            case R32_UINT                   :   return DXGI_FORMAT_R32_UINT                  ;
            case R32_SINT                   :   return DXGI_FORMAT_R32_SINT                  ;
            case R24G8_TYPELESS             :   return DXGI_FORMAT_R24G8_TYPELESS            ;
            case D24_UNORM_S8_UINT          :   return DXGI_FORMAT_D24_UNORM_S8_UINT         ;
            case R24_UNORM_X8_TYPELESS      :   return DXGI_FORMAT_R24_UNORM_X8_TYPELESS     ;
            case X24_TYPELESS_G8_UINT       :   return DXGI_FORMAT_X24_TYPELESS_G8_UINT      ;
            case R8G8_TYPELESS              :   return DXGI_FORMAT_R8G8_TYPELESS             ;
            case R8G8_UNORM                 :   return DXGI_FORMAT_R8G8_UNORM                ;
            case R8G8_UINT                  :   return DXGI_FORMAT_R8G8_UINT                 ;
            case R8G8_SNORM                 :   return DXGI_FORMAT_R8G8_SNORM                ;
            case R8G8_SINT                  :   return DXGI_FORMAT_R8G8_SINT                 ;
            case R16_TYPELESS               :   return DXGI_FORMAT_R16_TYPELESS              ;
            case R16_FLOAT                  :   return DXGI_FORMAT_R16_FLOAT                 ;
            case D16_UNORM                  :   return DXGI_FORMAT_D16_UNORM                 ;
            case R16_UNORM                  :   return DXGI_FORMAT_R16_UNORM                 ;
            case R16_UINT                   :   return DXGI_FORMAT_R16_UINT                  ;
            case R16_SNORM                  :   return DXGI_FORMAT_R16_SNORM                 ;
            case R16_SINT                   :   return DXGI_FORMAT_R16_SINT                  ;
            case R8_TYPELESS                :   return DXGI_FORMAT_R8_TYPELESS               ;
            case R8_UNORM                   :   return DXGI_FORMAT_R8_UNORM                  ;
            case R8_UINT                    :   return DXGI_FORMAT_R8_UINT                   ;
            case R8_SNORM                   :   return DXGI_FORMAT_R8_SNORM                  ;
            case R8_SINT                    :   return DXGI_FORMAT_R8_SINT                   ;
            case A8_UNORM                   :   return DXGI_FORMAT_A8_UNORM                  ;
            case R1_UNORM                   :   return DXGI_FORMAT_R1_UNORM                  ;
            case R9G9B9E5_SHAREDEXP         :   return DXGI_FORMAT_R9G9B9E5_SHAREDEXP        ;
            case R8G8_B8G8_UNORM            :   return DXGI_FORMAT_R8G8_B8G8_UNORM           ;
            case G8R8_G8B8_UNORM            :   return DXGI_FORMAT_G8R8_G8B8_UNORM           ;
            case BC1_TYPELESS               :   return DXGI_FORMAT_BC1_TYPELESS              ;
            case BC1_UNORM                  :   return DXGI_FORMAT_BC1_UNORM                 ;
            case BC1_UNORM_SRGB             :   return DXGI_FORMAT_BC1_UNORM_SRGB            ;
            case BC2_TYPELESS               :   return DXGI_FORMAT_BC2_TYPELESS              ;
            case BC2_UNORM                  :   return DXGI_FORMAT_BC2_UNORM                 ;
            case BC2_UNORM_SRGB             :   return DXGI_FORMAT_BC2_UNORM_SRGB            ;
            case BC3_TYPELESS               :   return DXGI_FORMAT_BC3_TYPELESS              ;
            case BC3_UNORM                  :   return DXGI_FORMAT_BC3_UNORM                 ;
            case BC3_UNORM_SRGB             :   return DXGI_FORMAT_BC3_UNORM_SRGB            ;
            case BC4_TYPELESS               :   return DXGI_FORMAT_BC4_TYPELESS              ;
            case BC4_UNORM                  :   return DXGI_FORMAT_BC4_UNORM                 ;
            case BC4_SNORM                  :   return DXGI_FORMAT_BC4_SNORM                 ;
            case BC5_TYPELESS               :   return DXGI_FORMAT_BC5_TYPELESS              ;
            case BC5_UNORM                  :   return DXGI_FORMAT_BC5_UNORM                 ;
            case BC5_SNORM                  :   return DXGI_FORMAT_BC5_SNORM                 ;
            case B5G6R5_UNORM               :   return DXGI_FORMAT_B5G6R5_UNORM              ;
            case B5G5R5A1_UNORM             :   return DXGI_FORMAT_B5G5R5A1_UNORM            ;
            case B8G8R8A8_UNORM             :   return DXGI_FORMAT_B8G8R8A8_UNORM            ;
            case B8G8R8X8_UNORM             :   return DXGI_FORMAT_B8G8R8X8_UNORM            ;
            case R10G10B10_XR_BIAS_A2_UNORM :   return DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM;
            case B8G8R8A8_TYPELESS          :   return DXGI_FORMAT_B8G8R8A8_TYPELESS         ;
            case B8G8R8A8_UNORM_SRGB        :   return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB       ;
            case B8G8R8X8_TYPELESS          :   return DXGI_FORMAT_B8G8R8X8_TYPELESS         ;
            case B8G8R8X8_UNORM_SRGB        :   return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB       ;
            case BC6H_TYPELESS              :   return DXGI_FORMAT_BC6H_TYPELESS             ;
            case BC6H_UF16                  :   return DXGI_FORMAT_BC6H_UF16                 ;
            case BC6H_SF16                  :   return DXGI_FORMAT_BC6H_SF16                 ;
            case BC7_TYPELESS               :   return DXGI_FORMAT_BC7_TYPELESS              ;
            case BC7_UNORM                  :   return DXGI_FORMAT_BC7_UNORM                 ;
            case BC7_UNORM_SRGB             :   return DXGI_FORMAT_BC7_UNORM_SRGB            ;
            case AYUV                       :   return DXGI_FORMAT_AYUV                      ;
            case Y410                       :   return DXGI_FORMAT_Y410                      ;
            case Y416                       :   return DXGI_FORMAT_Y416                      ;
            case NV12                       :   return DXGI_FORMAT_NV12                      ;
            case P010                       :   return DXGI_FORMAT_P010                      ;
            case P016                       :   return DXGI_FORMAT_P016                      ;
            case _420_OPAQUE                :   return DXGI_FORMAT_420_OPAQUE                ;
            case YUY2                       :   return DXGI_FORMAT_YUY2                      ;
            case Y210                       :   return DXGI_FORMAT_Y210                      ;
            case Y216                       :   return DXGI_FORMAT_Y216                      ;
            case NV11                       :   return DXGI_FORMAT_NV11                      ;
            case AI44                       :   return DXGI_FORMAT_AI44                      ;
            case IA44                       :   return DXGI_FORMAT_IA44                      ;
            case P8                         :   return DXGI_FORMAT_P8                        ;
            case A8P8                       :   return DXGI_FORMAT_A8P8                      ;
            case B4G4R4A4_UNORM             :   return DXGI_FORMAT_B4G4R4A4_UNORM            ;
            default                         :   return DXGI_FORMAT_UNKNOWN                   ;
        }
    }

    constexpr format::SurfaceFormat GetFormat(DXGI_FORMAT format)
    {
        using namespace format;
        switch (format)
        {
            case DXGI_FORMAT_UNKNOWN	                :   return UNKNOWN	                    ;
            case DXGI_FORMAT_R32G32B32A32_TYPELESS      :   return R32G32B32A32_TYPELESS        ;
            case DXGI_FORMAT_R32G32B32A32_FLOAT         :   return R32G32B32A32_FLOAT           ;
            case DXGI_FORMAT_R32G32B32A32_UINT          :   return R32G32B32A32_UINT            ;
            case DXGI_FORMAT_R32G32B32A32_SINT          :   return R32G32B32A32_SINT            ;
            case DXGI_FORMAT_R32G32B32_TYPELESS         :   return R32G32B32_TYPELESS           ;
            case DXGI_FORMAT_R32G32B32_FLOAT            :   return R32G32B32_FLOAT              ;
            case DXGI_FORMAT_R32G32B32_UINT             :   return R32G32B32_UINT               ;
            case DXGI_FORMAT_R32G32B32_SINT             :   return R32G32B32_SINT               ;
            case DXGI_FORMAT_R16G16B16A16_TYPELESS      :   return R16G16B16A16_TYPELESS        ;
            case DXGI_FORMAT_R16G16B16A16_FLOAT         :   return R16G16B16A16_FLOAT           ;
            case DXGI_FORMAT_R16G16B16A16_UNORM         :   return R16G16B16A16_UNORM           ;
            case DXGI_FORMAT_R16G16B16A16_UINT          :   return R16G16B16A16_UINT            ;
            case DXGI_FORMAT_R16G16B16A16_SNORM         :   return R16G16B16A16_SNORM           ;
            case DXGI_FORMAT_R16G16B16A16_SINT          :   return R16G16B16A16_SINT            ;
            case DXGI_FORMAT_R32G32_TYPELESS            :   return R32G32_TYPELESS              ;
            case DXGI_FORMAT_R32G32_FLOAT               :   return R32G32_FLOAT                 ;
            case DXGI_FORMAT_R32G32_UINT                :   return R32G32_UINT                  ;
            case DXGI_FORMAT_R32G32_SINT                :   return R32G32_SINT                  ;
            case DXGI_FORMAT_R32G8X24_TYPELESS          :   return R32G8X24_TYPELESS            ;
            case DXGI_FORMAT_D32_FLOAT_S8X24_UINT       :   return D32_FLOAT_S8X24_UINT         ;
            case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS   :   return R32_FLOAT_X8X24_TYPELESS     ;
            case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT    :   return X32_TYPELESS_G8X24_UINT      ;
            case DXGI_FORMAT_R10G10B10A2_TYPELESS       :   return R10G10B10A2_TYPELESS         ;
            case DXGI_FORMAT_R10G10B10A2_UNORM          :   return R10G10B10A2_UNORM            ;
            case DXGI_FORMAT_R10G10B10A2_UINT           :   return R10G10B10A2_UINT             ;
            case DXGI_FORMAT_R11G11B10_FLOAT            :   return R11G11B10_FLOAT              ;
            case DXGI_FORMAT_R8G8B8A8_TYPELESS          :   return R8G8B8A8_TYPELESS            ;
            case DXGI_FORMAT_R8G8B8A8_UNORM             :   return R8G8B8A8_UNORM               ;
            case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB        :   return R8G8B8A8_UNORM_SRGB          ;
            case DXGI_FORMAT_R8G8B8A8_UINT              :   return R8G8B8A8_UINT                ;
            case DXGI_FORMAT_R8G8B8A8_SNORM             :   return R8G8B8A8_SNORM               ;
            case DXGI_FORMAT_R8G8B8A8_SINT              :   return R8G8B8A8_SINT                ;
            case DXGI_FORMAT_R16G16_TYPELESS            :   return R16G16_TYPELESS              ;
            case DXGI_FORMAT_R16G16_FLOAT               :   return R16G16_FLOAT                 ;
            case DXGI_FORMAT_R16G16_UNORM               :   return R16G16_UNORM                 ;
            case DXGI_FORMAT_R16G16_UINT                :   return R16G16_UINT                  ;
            case DXGI_FORMAT_R16G16_SNORM               :   return R16G16_SNORM                 ;
            case DXGI_FORMAT_R16G16_SINT                :   return R16G16_SINT                  ;
            case DXGI_FORMAT_R32_TYPELESS               :   return R32_TYPELESS                 ;
            case DXGI_FORMAT_D32_FLOAT                  :   return D32_FLOAT                    ;
            case DXGI_FORMAT_R32_FLOAT                  :   return R32_FLOAT                    ;
            case DXGI_FORMAT_R32_UINT                   :   return R32_UINT                     ;
            case DXGI_FORMAT_R32_SINT                   :   return R32_SINT                     ;
            case DXGI_FORMAT_R24G8_TYPELESS             :   return R24G8_TYPELESS               ;
            case DXGI_FORMAT_D24_UNORM_S8_UINT          :   return D24_UNORM_S8_UINT            ;
            case DXGI_FORMAT_R24_UNORM_X8_TYPELESS      :   return R24_UNORM_X8_TYPELESS        ;
            case DXGI_FORMAT_X24_TYPELESS_G8_UINT       :   return X24_TYPELESS_G8_UINT         ;
            case DXGI_FORMAT_R8G8_TYPELESS              :   return R8G8_TYPELESS                ;
            case DXGI_FORMAT_R8G8_UNORM                 :   return R8G8_UNORM                   ;
            case DXGI_FORMAT_R8G8_UINT                  :   return R8G8_UINT                    ;
            case DXGI_FORMAT_R8G8_SNORM                 :   return R8G8_SNORM                   ;
            case DXGI_FORMAT_R8G8_SINT                  :   return R8G8_SINT                    ;
            case DXGI_FORMAT_R16_TYPELESS               :   return R16_TYPELESS                 ;
            case DXGI_FORMAT_R16_FLOAT                  :   return R16_FLOAT                    ;
            case DXGI_FORMAT_D16_UNORM                  :   return D16_UNORM                    ;
            case DXGI_FORMAT_R16_UNORM                  :   return R16_UNORM                    ;
            case DXGI_FORMAT_R16_UINT                   :   return R16_UINT                     ;
            case DXGI_FORMAT_R16_SNORM                  :   return R16_SNORM                    ;
            case DXGI_FORMAT_R16_SINT                   :   return R16_SINT                     ;
            case DXGI_FORMAT_R8_TYPELESS                :   return R8_TYPELESS                  ;
            case DXGI_FORMAT_R8_UNORM                   :   return R8_UNORM                     ;
            case DXGI_FORMAT_R8_UINT                    :   return R8_UINT                      ;
            case DXGI_FORMAT_R8_SNORM                   :   return R8_SNORM                     ;
            case DXGI_FORMAT_R8_SINT                    :   return R8_SINT                      ;
            case DXGI_FORMAT_A8_UNORM                   :   return A8_UNORM                     ;
            case DXGI_FORMAT_R1_UNORM                   :   return R1_UNORM                     ;
            case DXGI_FORMAT_R9G9B9E5_SHAREDEXP         :   return R9G9B9E5_SHAREDEXP           ;
            case DXGI_FORMAT_R8G8_B8G8_UNORM            :   return R8G8_B8G8_UNORM              ;
            case DXGI_FORMAT_G8R8_G8B8_UNORM            :   return G8R8_G8B8_UNORM              ;
            case DXGI_FORMAT_BC1_TYPELESS               :   return BC1_TYPELESS                 ;
            case DXGI_FORMAT_BC1_UNORM                  :   return BC1_UNORM                    ;
            case DXGI_FORMAT_BC1_UNORM_SRGB             :   return BC1_UNORM_SRGB               ;
            case DXGI_FORMAT_BC2_TYPELESS               :   return BC2_TYPELESS                 ;
            case DXGI_FORMAT_BC2_UNORM                  :   return BC2_UNORM                    ;
            case DXGI_FORMAT_BC2_UNORM_SRGB             :   return BC2_UNORM_SRGB               ;
            case DXGI_FORMAT_BC3_TYPELESS               :   return BC3_TYPELESS                 ;
            case DXGI_FORMAT_BC3_UNORM                  :   return BC3_UNORM                    ;
            case DXGI_FORMAT_BC3_UNORM_SRGB             :   return BC3_UNORM_SRGB               ;
            case DXGI_FORMAT_BC4_TYPELESS               :   return BC4_TYPELESS                 ;
            case DXGI_FORMAT_BC4_UNORM                  :   return BC4_UNORM                    ;
            case DXGI_FORMAT_BC4_SNORM                  :   return BC4_SNORM                    ;
            case DXGI_FORMAT_BC5_TYPELESS               :   return BC5_TYPELESS                 ;
            case DXGI_FORMAT_BC5_UNORM                  :   return BC5_UNORM                    ;
            case DXGI_FORMAT_BC5_SNORM                  :   return BC5_SNORM                    ;
            case DXGI_FORMAT_B5G6R5_UNORM               :   return B5G6R5_UNORM                 ;
            case DXGI_FORMAT_B5G5R5A1_UNORM             :   return B5G5R5A1_UNORM               ;
            case DXGI_FORMAT_B8G8R8A8_UNORM             :   return B8G8R8A8_UNORM               ;
            case DXGI_FORMAT_B8G8R8X8_UNORM             :   return B8G8R8X8_UNORM               ;
            case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM :   return R10G10B10_XR_BIAS_A2_UNORM   ;
            case DXGI_FORMAT_B8G8R8A8_TYPELESS          :   return B8G8R8A8_TYPELESS            ;
            case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB        :   return B8G8R8A8_UNORM_SRGB          ;
            case DXGI_FORMAT_B8G8R8X8_TYPELESS          :   return B8G8R8X8_TYPELESS            ;
            case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB        :   return B8G8R8X8_UNORM_SRGB          ;
            case DXGI_FORMAT_BC6H_TYPELESS              :   return BC6H_TYPELESS                ;
            case DXGI_FORMAT_BC6H_UF16                  :   return BC6H_UF16                    ;
            case DXGI_FORMAT_BC6H_SF16                  :   return BC6H_SF16                    ;
            case DXGI_FORMAT_BC7_TYPELESS               :   return BC7_TYPELESS                 ;
            case DXGI_FORMAT_BC7_UNORM                  :   return BC7_UNORM                    ;
            case DXGI_FORMAT_BC7_UNORM_SRGB             :   return BC7_UNORM_SRGB               ;
            case DXGI_FORMAT_AYUV                       :   return AYUV                         ;
            case DXGI_FORMAT_Y410                       :   return Y410                         ;
            case DXGI_FORMAT_Y416                       :   return Y416                         ;
            case DXGI_FORMAT_NV12                       :   return NV12                         ;
            case DXGI_FORMAT_P010                       :   return P010                         ;
            case DXGI_FORMAT_P016                       :   return P016                         ;
            case DXGI_FORMAT_420_OPAQUE                 :   return _420_OPAQUE                  ;
            case DXGI_FORMAT_YUY2                       :   return YUY2                         ;
            case DXGI_FORMAT_Y210                       :   return Y210                         ;
            case DXGI_FORMAT_Y216                       :   return Y216                         ;
            case DXGI_FORMAT_NV11                       :   return NV11                         ;
            case DXGI_FORMAT_AI44                       :   return AI44                         ;
            case DXGI_FORMAT_IA44                       :   return IA44                         ;
            case DXGI_FORMAT_P8                         :   return P8                           ;
            case DXGI_FORMAT_A8P8                       :   return A8P8                         ;
            case DXGI_FORMAT_B4G4R4A4_UNORM             :   return B4G4R4A4_UNORM               ;
            default                                     :   return UNKNOWN                      ;
        }
    }

    constexpr D3D12_RECT GetRect(const Rect& rect)
    {
        return D3D12_RECT{ .left = rect.mLeft, .top = rect.mTop, .right = rect.mRight, .bottom = rect.mBottom };
    }
}

#endif