#include <pch.h>
#include <DebugUI/DebugUIPass.h>

#if NV_ENABLE_DEBUG_UI

#include <Renderer/PipelineState.h>
#include <Renderer/ResourceManager.h>
#include <Renderer/Renderer.h>
#include <Renderer/Mesh.h>
#include <Renderer/Device.h>

#include "Imgui/imgui.h"

#if NV_RENDERER_DX12 // DX12 implementation only

#include "Imgui/imgui_impl_win32.h"
#include "Imgui/imgui_impl_dx12.h"

#include <DX12/DescriptorHeapDX12.h>
#include <DX12/RendererDX12.h>
#include <DX12/WindowDX12.h>
#include <DX12/DeviceDX12.h>
#include <DX12/ContextDX12.h>

namespace nv::graphics
{
    Handle<DescriptorHeap> mDescriptorHeapHandle;

    void DebugUIPass::Init()
    {
        auto window = (WindowDX12*)gWindow;
        auto renderer = (RendererDX12*)gRenderer;
        auto ctx = renderer->GetContext();
        auto device = (DeviceDX12*)renderer->GetDevice();

        DescriptorHeapDX12* heap = renderer->CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, mDescriptorHeapHandle, true);
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

        ImGui::StyleColorsDark();
        ImGui_ImplWin32_Init(window->GetWindowHandle());
        ImGui_ImplDX12_Init(device->GetDevice(), FRAMEBUFFER_COUNT,
            DXGI_FORMAT_R8G8B8A8_UNORM,
            heap->Get(),
            heap->HandleCPU(0),
            heap->HandleGPU(0));
    }

    static bool showDemoWindow = false;

    void DebugUIPass::Execute(const RenderPassData& renderPassData)
    {
        auto window = (WindowDX12*)gWindow;
        auto renderer = (RendererDX12*)gRenderer;
        auto ctx = (ContextDX12*)renderer->GetContext();
        auto device = (DeviceDX12*)renderer->GetDevice();

        ctx->SetDescriptorHeap({ mDescriptorHeapHandle });
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();

        ImGui::NewFrame();
        ImGuiIO& io = ImGui::GetIO();
        {
            static float f = 0.0f;

            ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &showDemoWindow);      // Edit bools storing our window open/close state

            static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }

        if(showDemoWindow)
            ImGui::ShowDemoWindow(&showDemoWindow);

        ImGui::Render();
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), ctx->GetCommandList());
        SetContextDefault(ctx);
    }

    void DebugUIPass::Destroy()
    {
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }
}

#endif // NV_RENDERER_DX12

#endif //NV_ENABLE_DEBUG_UI