#pragma once

#include <Renderer/Renderer.h>

namespace nv::graphics
{
    class RendererDX12 : public IRenderer
    {
    public:
        // Inherited via IRenderer
        virtual void Init(Window& window) override;
        virtual void Destroy() override;
        virtual void Present() override;
    private:

    };

    void ReportLeaksDX12();
}
