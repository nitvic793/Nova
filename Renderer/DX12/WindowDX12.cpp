#include "pch.h"
#include "WindowDX12.h"
#include <Windows.h>
#include "Keyboard.h"
#include "Mouse.h"
#include <windowsx.h>
#include <wrl.h>

LRESULT CALLBACK WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    auto window = static_cast<nv::graphics::WindowDX12*>(nv::graphics::gWindow);
    return window->WndProcHandler(hWnd, msg, wParam, lParam);
}

namespace nv::graphics
{
    bool WindowDX12::Init(uint32_t width, uint32_t height, bool fullscreen)
    {
        const wchar_t* windowName = L"Nova";
        const wchar_t* windowTitle = L"Nova";

        mWidth = width;
        mHeight = height;
        mbFullScreen = fullscreen;

        auto hr = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);
        if (FAILED(hr)) return false;

        HINSTANCE hInstance = GetModuleHandle(0);

        WNDCLASSEX wndClass;
        ZeroMemory(&wndClass, sizeof(WNDCLASSEX));
        wndClass.cbSize = sizeof(WNDCLASSEX);
        wndClass.style = CS_HREDRAW | CS_VREDRAW;
        wndClass.lpfnWndProc = ::WndProcHandler;
        wndClass.hInstance = hInstance;
        wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
        wndClass.hbrBackground = (HBRUSH)COLOR_WINDOW;
        wndClass.lpszClassName = windowName;

        RegisterClassEx(&wndClass);
        RECT clientRect;
        SetRect(&clientRect, 0, 0, width, height);
        AdjustWindowRect(
            &clientRect,
            WS_OVERLAPPEDWINDOW,	// Has a title bar, border, min and max buttons, etc.
            false);

        RECT desktopRect;
        GetClientRect(GetDesktopWindow(), &desktopRect);
        int centeredX = (desktopRect.right / 2) - (clientRect.right / 2);
        int centeredY = (desktopRect.bottom / 2) - (clientRect.bottom / 2);

        if (!mHwnd)
        {
            mHwnd = CreateWindow(
                wndClass.lpszClassName,
                windowTitle,
                WS_OVERLAPPEDWINDOW,
                centeredX,
                centeredY,
                clientRect.right - clientRect.left,	// Calculated width
                clientRect.bottom - clientRect.top,	// Calculated height
                0,			// No parent window
                0,			// No menu
                hInstance,	// The app's handle
                0);	  // used with multiple windows, NULL
        }

        if (fullscreen)
        {
            SetWindowLong(mHwnd, GWL_STYLE, 0);
        }

        int ShowWnd = 1;
        ShowWindow(mHwnd, ShowWnd);
        UpdateWindow(mHwnd);
        return true;
    }

    Window::ExecResult WindowDX12::ProcessMessages()
    {
        MSG msg = {};

        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (msg.message == WM_QUIT)
            return kNvQuit;

        return kNvNone;
    }

    LRESULT WindowDX12::WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        using namespace DirectX;
        switch (msg)
        {
        case WM_SIZE:
            mWidth = LOWORD(lParam);
            mHeight = HIWORD(lParam);
            OnResize();
            return 0;
        case WM_ACTIVATEAPP:
            Keyboard::ProcessMessage(msg, wParam, lParam);
            Mouse::ProcessMessage(msg, wParam, lParam);
            break;
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP:
            Keyboard::ProcessMessage(msg, wParam, lParam);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
            //OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            Mouse::ProcessMessage(msg, wParam, lParam);
            return 0;

            // Mouse button being released (while the cursor is currently over our window)
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
            //OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            Mouse::ProcessMessage(msg, wParam, lParam);
            return 0;

            // Cursor moves over the window (or outside, while we're currently capturing it)
        case WM_MOUSEMOVE:
            //OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            Mouse::ProcessMessage(msg, wParam, lParam);
            return 0;

            // Mouse wheel is scrolled
        case WM_MOUSEWHEEL:
            //OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            Mouse::ProcessMessage(msg, wParam, lParam);
            return 0;
        case WM_INPUT:
        case WM_XBUTTONDOWN:
        case WM_XBUTTONUP:
        case WM_MOUSEHOVER:
            Mouse::ProcessMessage(msg, wParam, lParam);
            break;
        case WM_MENUCHAR:
            // A menu is active and the user presses a key that does not correspond
            // to any mnemonic or accelerator key. Ignore so we don't produce an error beep.
            return MAKELRESULT(0, MNC_CLOSE);
        }

        return DefWindowProc(hWnd, msg, wParam, lParam);
    }

    void WindowDX12::SetInputWindow(void* pKeyboard, void* pMouse)
    {
        using namespace DirectX;
        auto kb = static_cast<Keyboard*>(pKeyboard);
        auto mouse = static_cast<Mouse*>(pMouse);

        mouse->SetWindow(mHwnd);
    }
}

