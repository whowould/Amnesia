#pragma once

#include <stop_token>

namespace amnesia::features
{
    auto run_movement(std::stop_token st) -> void;
}
