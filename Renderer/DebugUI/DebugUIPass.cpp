#include <pch.h>
#include <DebugUI/DebugUIPass.h>

#if NV_ENABLE_DEBUG_UI

#include <Renderer/PipelineState.h>
#include <Renderer/ResourceManager.h>
#include <Renderer/Renderer.h>
#include <Renderer/Mesh.h>
#include <Renderer/Device.h>
#include <Renderer/GlobalRenderSettings.h>
#include <Components/Renderable.h>
#include <Animation/Animation.h>
#include <Engine/EntityComponent.h>
#include <Engine/Camera.h>
#include <Math/Collision.h>
#include <Configs/MaterialDatabase.h>

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
    using namespace components;
    using namespace animation;

    static bool sbEnableDebugUI = true;
    static bool sbEnableDebugDraw = true;

    static uint32_t sgTexAllocIndex = 1; // 0 = render target

    Handle<DescriptorHeap> mDescriptorHeapHandle;

    void ListEntities(bool& open);
    void ShowAnimationPane(bool& open);

    static ImTextureID AllocTexture(Handle<Texture> tex, uint32_t& width, uint32_t& height)
    {
        auto renderer = (RendererDX12*)gRenderer;
        auto device = (DeviceDX12*)renderer->GetDevice();
        if (!tex.IsNull())
        {
            auto texDX12 = (TextureDX12*)gResourceManager->GetTexture(tex);
            const auto resourceDesc = texDX12->GetResource()->GetDesc();

            height = resourceDesc.Height ? (uint32_t)resourceDesc.Height : 320;
            width = resourceDesc.Width ? (uint32_t)resourceDesc.Width : 480;

            auto heap = renderer->GetDescriptorHeap(mDescriptorHeapHandle);
            device->GetDevice()->CopyDescriptorsSimple(1, heap->HandleCPU(sgTexAllocIndex), texDX12->GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            sgTexAllocIndex++;
            return (ImTextureID)heap->HandleGPU(sgTexAllocIndex - 1).ptr;
        }

        return nullptr;
    }

    static ImTextureID AllocTexture(StringID texId, uint32_t& width, uint32_t& height)
    {
        auto renderer = (RendererDX12*)gRenderer;
        auto device = (DeviceDX12*)renderer->GetDevice();
        if (gResourceTracker.ExistsTexture(texId))
        {
            return AllocTexture(gResourceManager->GetTextureHandle(texId), width, height);
        }

        return nullptr;
    }

    static void ShowTexturePreview(bool& open)
    {
        auto renderer = (RendererDX12*)gRenderer;
        auto device = (DeviceDX12*)renderer->GetDevice();

        ImGui::SetNextWindowBgAlpha(1.0f);
        ImGui::Begin("Texture Preview", &open);

        static StringID texId = 0;

        constexpr uint32_t BUFFER_SIZE = 1024;
        static char buffer[BUFFER_SIZE] = "RTPass/OutputBufferTex";
        ImGui::InputText("Texture Name", buffer, BUFFER_SIZE);

        uint32_t height;
        uint32_t width;
        texId = ID(buffer);

        auto texHandle = AllocTexture(texId, width, height);
        if(texHandle)
            ImGui::Image(texHandle, ImVec2((float)width, (float)height));

        ImGui::End();
    }

    static void ShowMaterialTextures(PBRMaterial* pMaterial, const char* pName)
    {
        const auto getMin = [](uint32_t wA, uint32_t hA, uint32_t wB, uint32_t hB)
        {
            using namespace std;
            return ImVec2((float)min(wA, wB), (float)min(hA, hB));
        };

        constexpr uint32_t MAX_WIDTH = 256;
        constexpr uint32_t MAX_HEIGHT = 256;

        if (ImGui::CollapsingHeader(pName, ImGuiTreeNodeFlags_DefaultOpen))
        {
            uint32_t width;
            uint32_t height;

            auto albedo = pMaterial->mAlbedoTexture;
            auto normal = pMaterial->mNormalTexture;
            auto metallic = pMaterial->mMetalnessTexture;
            auto roughness = pMaterial->mRoughnessTexture;

            ImGui::Text("Albedo");
            ImGui::SameLine();
            auto albedoTex = AllocTexture(albedo.mHash, width, height);
            if (albedoTex)
                ImGui::Image(albedoTex, getMin(width, height, MAX_WIDTH, MAX_HEIGHT));
            ImGui::SameLine();
            ImGui::Text("Normal");
            ImGui::SameLine();
            auto normalTex = AllocTexture(normal.mHash, width, height);
            if (normalTex)
                ImGui::Image(normalTex, getMin(width, height, MAX_WIDTH, MAX_HEIGHT));

            ImGui::Text("Metallic");
            ImGui::SameLine();
            auto metallicTex = AllocTexture(metallic.mHash, width, height);
            if (metallicTex)
                ImGui::Image(metallicTex, getMin(width, height, MAX_WIDTH, MAX_HEIGHT));
            ImGui::SameLine();
            ImGui::Text("Roughness");
            ImGui::SameLine();
            auto roughnessTex = AllocTexture(roughness.mHash, width, height);
            if (roughnessTex)
                ImGui::Image(roughnessTex, getMin(width, height, MAX_WIDTH, MAX_HEIGHT));
        }
    }

    static void ShowMaterialsPane(bool& open)
    {
        ImGui::Begin("Materials", &open);

        auto& materials = asset::gMaterialDatabase.mMaterials;
        for (auto& mat : materials)
        {
            ShowMaterialTextures(&mat.second, mat.first.data());
        }

        ImGui::End();
    }

    static void ShowRenderSettings()
    {
        if (ImGui::CollapsingHeader("Render Settings", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Checkbox("VSync", &gRenderSettings.mbEnableVSync);
            ImGui::Checkbox("Enable RT Shadows", &gRenderSettings.mbEnableRTShadows);
            ImGui::Checkbox("Enable RT DiffuseGI", &gRenderSettings.mbEnableRTDiffuseGI);
        }
	}

    void DebugUIPass::Init()
    {
        auto window = (WindowDX12*)gWindow;
        auto renderer = (RendererDX12*)gRenderer;
        auto ctx = renderer->GetContext();
        auto device = (DeviceDX12*)renderer->GetDevice();

        constexpr uint32_t IMGUI_DESCRIPTOR_HEAP_SIZE = 64;

        DescriptorHeapDX12* heap = renderer->CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, IMGUI_DESCRIPTOR_HEAP_SIZE, mDescriptorHeapHandle, true);
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
    static bool showAnimPane = false;
    static bool showMaterialPane = false;

    void DebugUIPass::Execute(const RenderPassData& renderPassData)
    {
        sgTexAllocIndex = 1;
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
            ImGui::Checkbox("Debug Draw", &sbEnableDebugDraw);
            ImGui::Checkbox("Entity Manager", &showEntityList);
            ImGui::Checkbox("Animation", &showAnimPane);
            ImGui::Checkbox("Materials", &showMaterialPane);
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ShowRenderSettings();
            ImGui::End();
        }

        if(showEntityList)
            ListEntities(showEntityList);

        if (showTexturePreview)
            ShowTexturePreview(showTexturePreview);

        if (showAnimPane)
            ShowAnimationPane(showAnimPane);

        if (showMaterialPane)
            ShowMaterialsPane(showMaterialPane);

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

    void Visit(uint32_t* pVal, const MetaField& field, const char* pCompName)
    {
        ImGui::PushID(pCompName);
        ImGui::DragInt(field.mName.c_str(), (int*)pVal, 0.05f);
        ImGui::PopID();
    }

    void Visit(float* pVal, const MetaField& field, const char* pCompName)
    {
        ImGui::PushID(pCompName);
        ImGui::DragFloat(field.mName.c_str(), pVal, 0.05f);
        ImGui::PopID();
    }

    void Visit(bool* pVal, const MetaField& field, const char* pCompName)
    {
        ImGui::PushID(pCompName);
        ImGui::Checkbox(field.mName.c_str(), pVal);
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
        const char* pName = field.mName.c_str();

        ImGui::PushID(pCompName);
        if (field.mName.find("Color") != std::string::npos)
            ImGui::ColorEdit3(pName, &pVal->x);
        else
            ImGui::DragFloat3(pName, &pVal->x, 0.05f);
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
                case FIELD_UINT:
                    Visit((uint32_t*)pCurrent, field, pName); break;
                case FIELD_BOOL:
                    Visit((bool*)pCurrent, field, pName); break;
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
                    if (field.mType == "DirectX::BoundingBox")
                    {
                        offset += sizeof(DirectX::BoundingBox);
                        auto bbox = (DirectX::BoundingBox*)pCurrent;

                        MetaField center = { .mName = "Center" };
                        MetaField extents = { .mName = "Extents" };

                        Visit((float3*)&bbox->Extents, extents, pName);
                        Visit((float3*)&bbox->Center, center, pName);
                    }
                    if (field.mType == "DirectX::BoundingSphere")
                    {
                        offset += sizeof(DirectX::BoundingSphere);
                        auto bsphere = (DirectX::BoundingSphere*)pCurrent;

                        MetaField center = { .mName = "Center" };
                        MetaField radius = { .mName = "Radius" };

                        Visit((float3*)&bsphere->Radius, center, pName);
                        Visit((float3*)&bsphere->Center, radius, pName);
                    }
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

        const auto drawComponent = [](StringID compId, IComponent* component)
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

    static Handle<Entity> gSelectedEntity = Null<Entity>();

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
                    gSelectedEntity = e.second;
                    static bool showComponents = true;
                    ListComponents(e.second, showComponents);
                }

                idx++;
            }
            ImGui::EndListBox();
        }
        ImGui::End();
    }

    void ShowAnimationPane(bool& open)
    {
        if (gSelectedEntity.IsNull())
            return;

        ImGui::Begin("Animation", &open);
        auto names = gAnimManager.GetAnimationNames();
        Entity* pEntity = gEntityManager.GetEntity(gSelectedEntity);
        AnimationComponent* pAnimComp = pEntity->Has<AnimationComponent>() ? pEntity->Get<AnimationComponent>() : nullptr;

        static const char* currentAnim = NULL;

        if (pAnimComp)
        {
            const std::string& currentEntityAnim = gAnimManager.GetAnimationName(pAnimComp->mCurrentAnimationIndex);
            currentAnim = currentEntityAnim.c_str();
        }
        else
        {
            currentAnim = nullptr;
        }
        
        if (ImGui::BeginCombo("##animCombo", currentAnim)) // The second parameter is the label previewed before opening the combo.
        {
            for (int n = 0; n < names.size(); n++)
            {
                bool bIsSelected = (names[n] == (currentAnim ? currentAnim : "")); // You can store your selection however you want, outside or inside your objects
                if (ImGui::Selectable(names[n].data(), bIsSelected))
                    currentAnim = names[n].data();
                    if (bIsSelected)
                        ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
            }
            ImGui::EndCombo();
        }


        if (pAnimComp)
        {
            if (currentAnim)
            {
                auto animIdx = gAnimManager.GetAnimationIndex(currentAnim);
                pAnimComp->mCurrentAnimationIndex = animIdx;
            }
        }

        ImGui::End();
    }


    void SetEnableDebugDraw(bool enable)
    {
        sbEnableDebugDraw = enable;
    }

    void SetEnableDebugUI(bool enable)
    {
        sbEnableDebugUI = enable;
    }

    bool IsDebugUIEnabled()
    {
        return sbEnableDebugUI;
    }

    bool IsDebugDrawEnabled()
    {
        return sbEnableDebugDraw;
    }
}

#endif // NV_RENDERER_DX12

#endif //NV_ENABLE_DEBUG_UI