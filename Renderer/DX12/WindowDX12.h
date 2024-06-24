#pragma once

#include <Renderer/Window.h>
#include <atomic>

struct HWND__;
using HWND = HWND__*;
using UINT = uint32_t;
using WPARAM = uint64_t;
using LPARAM = int64_t;
using LRESULT = int64_t;

#ifndef CALLBACK
#define CALLBACK    __stdcall
#endif

namespace nv::graphics
{
    class WindowDX12 : public Window
    {
    public:
        WindowDX12(HWND hwnd) :
            mHwnd(hwnd),
            mExecResult(kNvNone){}

        WindowDX12() :
            mHwnd(NULL),
            mExecResult(kNvNone) {}

        constexpr HWND GetWindowHandle() const { return mHwnd; }

        // Inherited via Window
        virtual bool Init(uint32_t width, uint32_t height, bool fullscreen) override;
        virtual ExecResult ProcessMessages() override;

        LRESULT CALLBACK WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
        void OnResize();

        void SetInputWindow(void* pKeyboard, void* pMouse);
        virtual ExecResult GetExecResult() const override { return mExecResult; }

    private:
        HWND mHwnd;
        std::atomic<ExecResult> mExecResult;
    };
}