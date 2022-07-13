#pragma once

#include <NovaConfig.h>
#if NV_RENDERER_DX12

#include <Renderer/CommonDefines.h>
#include <d3d12.h>

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
        }

        return D3D12_RESOURCE_STATE_COMMON;
    }
}

#endif