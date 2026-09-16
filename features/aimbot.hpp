#pragma once

#include <stop_token>

namespace amnesia::features
{
    auto run_aimbot(std::stop_token st) -> void;
    auto draw_aimbot() -> void;
}
