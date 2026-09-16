#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace amnesia::console
{
    auto init() -> void;
    auto banner() -> void;

    auto info(std::string_view channel, std::string_view text) -> void;
    auto ok(std::string_view channel, std::string_view text) -> void;
    auto fail(std::string_view channel, std::string_view text) -> void;
    auto warn(std::string_view channel, std::string_view text) -> void;

    auto hex(std::string_view channel, std::string_view label, std::uintptr_t value) -> void;
    auto dec(std::string_view channel, std::string_view label, std::int64_t value) -> void;
}
