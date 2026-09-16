#include "overlay.hpp"
#include "tahoma.hpp"

#include "../features/aimbot.hpp"
#include "../features/esp.hpp"
#include "../features/menu.hpp"
#include "../sdk/console.hpp"
#include "../sdk/mem.hpp"
#include "../sdk/settings.hpp"

#include "imgui.h"
#include "imgui_freetype.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <d3d11.h>
#include <dcomp.h>
#include <dwmapi.h>
#include <dxgi1_2.h>

#include <chrono>
#include <string>
#include <thread>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dcomp.lib")
#pragma comment(lib, "dwmapi.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

namespace amnesia::overlay
{
    namespace
    {
        constexpr wchar_t class_name[] = L"folkvalley1337classnameweudtoponesigma";

        HWND overlay_hwnd = nullptr;
        HWND roblox_hwnd = nullptr;
        ID3D11Device* device = nullptr;
        ID3D11DeviceContext* context = nullptr;
        IDXGISwapChain1* swapchain = nullptr;
        ID3D11RenderTargetView* rtv = nullptr;
        IDCompositionDevice* dcomp = nullptr;
        IDCompositionTarget* dcomp_target = nullptr;
        IDCompositionVisual* dcomp_visual = nullptr;
        bool running = true;
        LONG last_exstyle = 0;
        int swap_w = 0;
        int swap_h = 0;
        ImFont* name_font_ptr = nullptr;

        auto load_fonts(ImGuiIO& io) -> void
        {
            io.Fonts->Clear();
            name_font_ptr = nullptr;

            ImFontConfig menu_cfg{};
            menu_cfg.OversampleH = 1;
            menu_cfg.OversampleV = 1;
            menu_cfg.PixelSnapH = true;
            auto* menu = io.Fonts->AddFontDefault(&menu_cfg);
            io.FontDefault = menu;

            ImFontConfig cfg{};
            cfg.OversampleH = 1;
            cfg.OversampleV = 1;
            cfg.PixelSnapH = true;
            cfg.FontDataOwnedByAtlas = false;
            cfg.FontLoaderFlags = ImGuiFreeTypeLoaderFlags_MonoHinting;

            auto* loaded = io.Fonts->AddFontFromMemoryTTF(
                const_cast<unsigned char*>(tahoma_8pt_bold),
                static_cast<int>(tahoma_8pt_bold_size),
                13.f,
                &cfg);

            if (loaded)
                console::ok("overlay", "name font embedded tahoma 8pt bold");
            else
            {
                loaded = menu;
                console::warn("overlay", "embedded tahoma failed, using default font");
            }

            name_font_ptr = loaded;
            io.Fonts->Build();
        }

        auto find_roblox() -> HWND
        {
            const auto pid = mem::get().get_pid();
            if (!pid)
                return FindWindowW(nullptr, L"Roblox");

            struct ctx_t { DWORD pid; HWND hwnd; };
            ctx_t ctx{ pid, nullptr };
            ::EnumWindows([](HWND hwnd, LPARAM lp) -> BOOL
            {
                auto* ctx = reinterpret_cast<ctx_t*>(lp);
                DWORD window_pid = 0;
                ::GetWindowThreadProcessId(hwnd, &window_pid);
                if (window_pid != ctx->pid)
                    return TRUE;
                if (!::IsWindowVisible(hwnd))
                    return TRUE;
                wchar_t title[64]{};
                ::GetWindowTextW(hwnd, title, 64);
                if (title[0] == 0)
                    return TRUE;
                ctx->hwnd = hwnd;
                return FALSE;
            }, reinterpret_cast<LPARAM>(&ctx));

            if (ctx.hwnd)
                return ctx.hwnd;
            return FindWindowW(nullptr, L"Roblox");
        }

        auto destroy_rtv() -> void
        {
            if (rtv)
            {
                rtv->Release();
                rtv = nullptr;
            }
        }

        auto create_rtv() -> bool
        {
            destroy_rtv();
            ID3D11Texture2D* back = nullptr;
            if (FAILED(swapchain->GetBuffer(0, IID_PPV_ARGS(&back))) || !back)
                return false;
            const auto hr = device->CreateRenderTargetView(back, nullptr, &rtv);
            back->Release();
            return SUCCEEDED(hr);
        }

        auto bind_composition() -> bool
        {
            if (!dcomp || !swapchain)
                return false;

            if (dcomp_visual)
            {
                dcomp_visual->Release();
                dcomp_visual = nullptr;
            }

            if (FAILED(dcomp->CreateVisual(&dcomp_visual)))
                return false;
            if (FAILED(dcomp_visual->SetContent(swapchain)))
                return false;
            if (FAILED(dcomp_target->SetRoot(dcomp_visual)))
                return false;
            return SUCCEEDED(dcomp->Commit());
        }

        auto create_device(HWND window) -> bool
        {
            RECT rc{};
            ::GetClientRect(window, &rc);
            swap_w = rc.right - rc.left;
            swap_h = rc.bottom - rc.top;
            if (swap_w <= 0)
                swap_w = ::GetSystemMetrics(SM_CXSCREEN);
            if (swap_h <= 0)
                swap_h = ::GetSystemMetrics(SM_CYSCREEN);

            constexpr D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
            D3D_FEATURE_LEVEL got{};
            if (FAILED(D3D11CreateDevice(
                nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                levels, 2, D3D11_SDK_VERSION, &device, &got, &context)))
            {
                return false;
            }

            IDXGIDevice* dxgi_device = nullptr;
            if (FAILED(device->QueryInterface(IID_PPV_ARGS(&dxgi_device))))
                return false;

            IDXGIAdapter* adapter = nullptr;
            IDXGIFactory2* factory = nullptr;
            const auto adapter_hr = dxgi_device->GetAdapter(&adapter);
            if (SUCCEEDED(adapter_hr) && adapter)
                adapter->GetParent(IID_PPV_ARGS(&factory));

            DXGI_SWAP_CHAIN_DESC1 sd{};
            sd.Width = static_cast<UINT>(swap_w);
            sd.Height = static_cast<UINT>(swap_h);
            sd.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            sd.SampleDesc.Count = 1;
            sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            sd.BufferCount = 2;
            sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
            sd.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;
            sd.Scaling = DXGI_SCALING_STRETCH;

            HRESULT hr = E_FAIL;
            if (factory)
                hr = factory->CreateSwapChainForComposition(device, &sd, nullptr, &swapchain);

            if (adapter)
                adapter->Release();
            if (factory)
                factory->Release();

            if (FAILED(hr) || !swapchain)
            {
                dxgi_device->Release();
                return false;
            }

            hr = DCompositionCreateDevice(dxgi_device, IID_PPV_ARGS(&dcomp));
            dxgi_device->Release();
            if (FAILED(hr) || !dcomp)
                return false;

            if (FAILED(dcomp->CreateTargetForHwnd(window, TRUE, &dcomp_target)))
                return false;

            if (!create_rtv())
                return false;
            return bind_composition();
        }

        auto resize_buffers(int w, int h) -> void
        {
            if (!swapchain || !device || w <= 0 || h <= 0)
                return;
            if (w == swap_w && h == swap_h)
                return;

            destroy_rtv();
            if (FAILED(swapchain->ResizeBuffers(0, static_cast<UINT>(w), static_cast<UINT>(h), DXGI_FORMAT_B8G8R8A8_UNORM, 0)))
                return;
            swap_w = w;
            swap_h = h;
            create_rtv();
            bind_composition();
        }

        auto cleanup_device() -> void
        {
            destroy_rtv();
            if (dcomp_visual) { dcomp_visual->Release(); dcomp_visual = nullptr; }
            if (dcomp_target) { dcomp_target->Release(); dcomp_target = nullptr; }
            if (dcomp) { dcomp->Release(); dcomp = nullptr; }
            if (swapchain) { swapchain->Release(); swapchain = nullptr; }
            if (context) { context->Release(); context = nullptr; }
            if (device) { device->Release(); device = nullptr; }
        }

        auto move_overlay() -> void
        {
            if (!roblox_hwnd || !overlay_hwnd)
                return;

            RECT client{};
            if (!::GetClientRect(roblox_hwnd, &client))
                return;

            POINT origin{ client.left, client.top };
            if (!::ClientToScreen(roblox_hwnd, &origin))
                return;

            const auto x = origin.x;
            const auto y = origin.y;
            const auto w = client.right - client.left;
            const auto h = client.bottom - client.top;
            if (w <= 0 || h <= 0)
                return;

            static int lx = 0, ly = 0, lw = 0, lh = 0;
            if (x == lx && y == ly && w == lw && h == lh)
                return;
            lx = x; ly = y; lw = w; lh = h;
            ::MoveWindow(overlay_hwnd, x, y, w, h, TRUE);
            resize_buffers(w, h);
        }

        auto overlay_exstyle(bool click_through) -> LONG
        {
            LONG style = WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_NOREDIRECTIONBITMAP | WS_EX_LAYERED;
            if (click_through)
                style |= WS_EX_TRANSPARENT;
            return style;
        }

        auto set_click_through(bool click_through) -> void
        {
            const auto style = overlay_exstyle(click_through);
            if (style == last_exstyle)
                return;
            ::SetWindowLongW(overlay_hwnd, GWL_EXSTYLE, style);
            ::SetWindowPos(overlay_hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_NOSENDCHANGING);
            last_exstyle = style;
        }

        auto wndproc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT
        {
            if (ImGui_ImplWin32_WndProcHandler(window, msg, wparam, lparam))
                return 1;

            switch (msg)
            {
            case WM_ERASEBKGND:
                return 1;
            case WM_PAINT:
            {
                PAINTSTRUCT ps{};
                ::BeginPaint(window, &ps);
                ::EndPaint(window, &ps);
                return 0;
            }
            case WM_SIZE:
                if (swapchain && wparam != SIZE_MINIMIZED)
                    resize_buffers(LOWORD(lparam), HIWORD(lparam));
                return 0;
            case WM_DESTROY:
                running = false;
                ::PostQuitMessage(0);
                return 0;
            default:
                break;
            }
            return ::DefWindowProcW(window, msg, wparam, lparam);
        }
    }

    auto run() -> int
    {
        roblox_hwnd = find_roblox();
        if (!roblox_hwnd)
        {
            console::fail("overlay", "roblox window not found");
            return 1;
        }

        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = wndproc;
        wc.hInstance = ::GetModuleHandleW(nullptr);
        wc.lpszClassName = class_name;
        wc.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
        if (!::RegisterClassExW(&wc))
            return 1;

        RECT client{};
        ::GetClientRect(roblox_hwnd, &client);
        POINT origin{ 0, 0 };
        ::ClientToScreen(roblox_hwnd, &origin);
        auto w = client.right - client.left;
        auto h = client.bottom - client.top;
        if (w <= 0)
            w = ::GetSystemMetrics(SM_CXSCREEN);
        if (h <= 0)
            h = ::GetSystemMetrics(SM_CYSCREEN);

        overlay_hwnd = ::CreateWindowExW(
            overlay_exstyle(true),
            class_name, L"amnesia",
            WS_POPUP,
            origin.x, origin.y, w, h,
            nullptr, nullptr, wc.hInstance, nullptr);
        if (!overlay_hwnd)
        {
            ::UnregisterClassW(class_name, wc.hInstance);
            return 1;
        }
        last_exstyle = overlay_exstyle(true);

        if (!create_device(overlay_hwnd))
        {
            console::fail("overlay", "d3d11 composition init failed");
            ::DestroyWindow(overlay_hwnd);
            ::UnregisterClassW(class_name, wc.hInstance);
            return 1;
        }

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        auto& io = ImGui::GetIO();
        io.IniFilename = nullptr;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        load_fonts(io);

        ImGui::StyleColorsDark();
        auto& style = ImGui::GetStyle();
        style.WindowRounding = 8.f;
        style.FrameRounding = 4.f;
        style.GrabRounding = 4.f;
        style.Colors[ImGuiCol_WindowBg].w = 0.94f;

        ImGui_ImplWin32_Init(overlay_hwnd);
        ImGui_ImplDX11_Init(device, context);

        ::ShowWindow(overlay_hwnd, SW_SHOWNOACTIVATE);
        ::UpdateWindow(overlay_hwnd);
        set_click_through(true);
        move_overlay();

        console::ok("overlay", "d3d11 overlay online  —  INSERT toggles menu");

        MSG msg{};
        while (running)
        {
            while (::PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                ::TranslateMessage(&msg);
                ::DispatchMessageW(&msg);
                if (msg.message == WM_QUIT)
                    running = false;
            }
            if (!running)
                break;

            if ((::GetAsyncKeyState(VK_INSERT) & 1) != 0)
            {
                settings::menu_open = !settings::menu_open;
                last_exstyle = 0;
            }

            roblox_hwnd = find_roblox();
            if (!roblox_hwnd)
            {
                console::warn("overlay", "roblox closed");
                running = false;
                break;
            }

            move_overlay();
            set_click_through(!settings::menu_open);

            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            features::draw_esp();
            features::draw_aimbot();
            features::draw_menu();

            ImGui::Render();
            constexpr float clear[4]{ 0.f, 0.f, 0.f, 0.f };
            context->OMSetRenderTargets(1, &rtv, nullptr);
            context->ClearRenderTargetView(rtv, clear);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
            swapchain->Present(1, 0);

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        cleanup_device();
        ::DestroyWindow(overlay_hwnd);
        ::UnregisterClassW(class_name, wc.hInstance);
        overlay_hwnd = nullptr;
        name_font_ptr = nullptr;
        return 0;
    }

    auto name_font() -> ImFont*
    {
        return name_font_ptr;
    }

    auto roblox_window() -> HWND
    {
        return roblox_hwnd;
    }
}
