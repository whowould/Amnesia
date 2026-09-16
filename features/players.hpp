#pragma once

#include <stop_token>

namespace amnesia::features
{
    auto log_players(std::stop_token st) -> void;
}
