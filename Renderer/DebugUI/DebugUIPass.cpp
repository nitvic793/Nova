#include <pch.h>
#include <DebugUI/DebugUIPass.h>

#if NV_ENABLE_DEBUG_UI

#include <Renderer/PipelineState.h>
#include <Renderer/ResourceManager.h>
#include <Renderer/Renderer.h>
#include <Renderer/Mesh.h>
#include <Renderer/Device.h>
#include <Engine/EntityComponent.h>
#include <Engine/Camera.h>

#include "Imgui/imgui.h"

#if NV_RENDERER_DX12 // DX12 implementation only

#include "Imgui/imgui_impl_win32.h"
#include "Imgui/imgui_impl_dx12.h"

#include <DX12/DescriptorHeapDX12.h>
#include <DX12/RendererDX12.h>
#include <DX12/WindowDX12.h>
#include <DX12/DeviceDX12.h>
#include <DX12/ContextDX12.h>
#include <DX12/TextureDX12.h>

using namespace nv::ecs;

namespace nv::graphics
{
    static bool sbEnableDebugUI = true;

    Handle<DescriptorHeap> mDescriptorHeapHandle;

    void ListEntities(bool& open);

    static void ShowTexturePreview(bool& open)
    {
        auto renderer = (RendererDX12*)gRenderer;
        auto device = (DeviceDX12*)renderer->GetDevice();

        ImGui::Begin("Texture Preview", &open);

        static StringID texId = 0;

        constexpr uint32_t BUFFER_SIZE = 1024;
        static char buffer[BUFFER_SIZE] = "RTPass/OutputBufferTex";
        ImGui::InputText("Texture Name", buffer, BUFFER_SIZE);
        texId = ID(buffer);
        if (gResourceTracker.ExistsTexture(texId))
        {
            auto tex = (TextureDX12*)gResourceManager->GetTexture(texId);
            auto heap = renderer->GetDescriptorHeap(mDescriptorHeapHandle);
            device->GetDevice()->CopyDescriptorsSimple(1, heap->HandleCPU(1), tex->GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            ImGui::Image((ImTextureID)heap->HandleGPU(1).ptr, ImVec2(480, 320));
        }
        ImGui::End();
    }

    void DebugUIPass::Init()
    {
        auto window = (WindowDX12*)gWindow;
        auto renderer = (RendererDX12*)gRenderer;
        auto ctx = renderer->GetContext();
        auto device = (DeviceDX12*)renderer->GetDevice();

        DescriptorHeapDX12* heap = renderer->CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 2, mDescriptorHeapHandle, true);
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
    static bool showTexturePreview = false;
    static bool showEntityList = false;

    void DebugUIPass::Execute(const RenderPassData& renderPassData)
    {
        if (!sbEnableDebugUI)
            return;

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
            ImGui::Begin("Nova");                                 
            ImGui::Checkbox("Texture Preview", &showTexturePreview);
            ImGui::Checkbox("Entity Manager", &showEntityList);
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }

        if(showEntityList)
            ListEntities(showEntityList);

        if (showTexturePreview)
            ShowTexturePreview(showTexturePreview);

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

    void Visit(int32_t* pVal, const MetaField& field, const char* pCompName)
    {
        ImGui::PushID(pCompName);
        ImGui::DragInt(field.mName.c_str(), pVal, 0.05f);
        ImGui::PopID();
    }

    void Visit(float* pVal, const MetaField& field, const char* pCompName)
    {
        ImGui::PushID(pCompName);
        ImGui::DragFloat(field.mName.c_str(), pVal, 0.05f);
        ImGui::PopID();
    }

    void Visit(float2* pVal, const MetaField& field, const char* pCompName)
    {
        ImGui::PushID(pCompName);
        ImGui::DragFloat2(field.mName.c_str(), &pVal->x, 0.05f);
        ImGui::PopID();
    }

    void Visit(float3* pVal, const MetaField& field, const char* pCompName)
    {
        ImGui::PushID(pCompName);
        ImGui::DragFloat3(field.mName.c_str(), &pVal->x, 0.05f);
        ImGui::PopID();
    }

    void Visit(float4* pVal, const MetaField& field, const char* pCompName)
    {
        ImGui::PushID(pCompName);
        ImGui::DragFloat4(field.mName.c_str(), &pVal->x, 0.05f);
        ImGui::PopID();
    }

    void Visit(std::string* pVal, const MetaField& field, const char* pCompName)
    {
        ImGui::PushID(pCompName);
        ImGui::Text("%s: %s", field.mName.c_str(), pVal->data());
        ImGui::PopID();
    }

    void VisitFields(const std::vector<MetaField>& fields, const char* pName, IComponent* pComponent)
    {
        size_t offset = 0;
        const uint8_t* pBuffer = (uint8_t*)pComponent;
        if (ImGui::CollapsingHeader(pName, ImGuiTreeNodeFlags_DefaultOpen))
        {
            for (auto& field : fields)
            {
                const size_t fieldSize = gFieldSizeMap[field.mFieldType];
                const uint8_t* pCurrent = pBuffer + offset;
                switch (field.mFieldType)
                {
                case FIELD_INT:
                    Visit((int32_t*)pCurrent, field, pName); break;
                case FIELD_FLOAT:
                    Visit((float*)pCurrent, field, pName); break;
                case FIELD_FLOAT2:                         
                    Visit((float2*)pCurrent, field, pName); break;
                case FIELD_FLOAT3:                       
                    Visit((float3*)pCurrent, field, pName); break;
                case FIELD_FLOAT4:                        
                    Visit((float4*)pCurrent, field, pName); break;
                case FIELD_STRING:
                    Visit((std::string*)pCurrent, field, pName); break;
                case FIELD_UNDEFINED:
                    if (field.mType == "Camera")
                        offset += sizeof(Camera);
                    break;
                default:
                    // Do Nothing
                    break;
                }

                offset += fieldSize;
            }
        }
    }

    void ListComponents(Handle<Entity> entityHandle, bool& open)
    {
        Entity* e = gEntityManager.GetEntity(entityHandle);
        std::unordered_map<StringID, IComponent*> components;
        e->GetComponents(components);

        const auto& drawComponent = [&](StringID compId, IComponent* component)
        {
            std::string name;
            const auto& fields = gComponentManager.GetMetadata(compId, name);
            VisitFields(fields, name.c_str(), component);
        };

        ImGui::Begin("Components", &open);
        for (const auto& comp : components)
            drawComponent(comp.first, comp.second);

        ImGui::End();
    }

    void ListEntities(bool& open)
    {
        auto& nameMap = gEntityManager.GetEntityNameMap();

        ImGui::Begin("Entity Manager", &open);
        if (ImGui::BeginListBox("Entities"))
        {
            static int32_t selectedIdx = 0;
            uint32_t idx = 0;
            for (auto& e : nameMap)
            {
                const bool selected = idx == selectedIdx;
                if (ImGui::Selectable(e.first.c_str(), selected))
                {    
                    selectedIdx = idx;
                }

                if (selected)
                {
                    static bool showComponents = true;
                    ListComponents(e.second, showComponents);
                }

                idx++;
            }
            ImGui::EndListBox();
        }
        ImGui::End();
    }


    void SetEnableDebugUI(bool enable)
    {
        sbEnableDebugUI = enable;
    }

    bool IsDebugUIEnabled()
    {
        return sbEnableDebugUI;
    }
}

#endif // NV_RENDERER_DX12

#endif //NV_ENABLE_DEBUG_UI