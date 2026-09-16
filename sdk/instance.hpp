#pragma once

#include "math.hpp"
#include "mem.hpp"
#include "offsets.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace amnesia
{
    class instance
    {
        std::uintptr_t addr_{ 0 };

    public:
        instance() = default;
        explicit instance(std::uintptr_t addr) : addr_(addr) {}

        [[nodiscard]] auto address() const -> std::uintptr_t { return addr_; }
        [[nodiscard]] auto valid() const -> bool { return mem::is_valid(addr_); }
        explicit operator bool() const { return valid(); }

        [[nodiscard]] auto name() const -> std::string;
        [[nodiscard]] auto classname() const -> std::string;
        [[nodiscard]] auto parent() const -> instance;

        [[nodiscard]] auto children() const -> std::vector<instance>;
        [[nodiscard]] auto find(std::string_view name) const -> instance;
        [[nodiscard]] auto find_class(std::string_view classname) const -> instance;

        template<typename T>
        [[nodiscard]] auto read(std::uintptr_t offset) const -> T
        {
            return mem::get().read<T>(addr_ + offset);
        }

        template<typename T>
        auto write(std::uintptr_t offset, const T& value) const -> void
        {
            mem::get().write<T>(addr_ + offset, value);
        }
    };

    [[nodiscard]] inline auto local_player(const instance& players) -> instance
    {
        return instance{ players.read<std::uintptr_t>(Offsets::Player::LocalPlayer) };
    }

    [[nodiscard]] inline auto character(const instance& player) -> instance
    {
        return instance{ player.read<std::uintptr_t>(Offsets::Player::ModelInstance) };
    }

    [[nodiscard]] inline auto primitive(const instance& part) -> std::uintptr_t
    {
        return part.read<std::uintptr_t>(Offsets::BasePart::Primitive);
    }
}
