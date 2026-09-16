#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

struct ImFont;

namespace amnesia::overlay
{
    auto run() -> int;
    [[nodiscard]] auto name_font() -> ImFont*;
    [[nodiscard]] auto roblox_window() -> HWND;
}
