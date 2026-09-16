#pragma once

#include <cmath>
#include <cstdint>

namespace amnesia
{
    struct vector2_t
    {
        float x{};
        float y{};

        [[nodiscard]] constexpr auto operator+(const vector2_t& o) const -> vector2_t { return { x + o.x, y + o.y }; }
        [[nodiscard]] constexpr auto operator-(const vector2_t& o) const -> vector2_t { return { x - o.x, y - o.y }; }
        [[nodiscard]] constexpr auto operator*(float s) const -> vector2_t { return { x * s, y * s }; }
        [[nodiscard]] auto length() const -> float { return std::sqrt(x * x + y * y); }
    };

    struct vector3_t
    {
        float x{};
        float y{};
        float z{};

        [[nodiscard]] constexpr auto operator+(const vector3_t& o) const -> vector3_t { return { x + o.x, y + o.y, z + o.z }; }
        [[nodiscard]] constexpr auto operator-(const vector3_t& o) const -> vector3_t { return { x - o.x, y - o.y, z - o.z }; }
        [[nodiscard]] constexpr auto operator*(float s) const -> vector3_t { return { x * s, y * s, z * s }; }
        [[nodiscard]] constexpr auto dot(const vector3_t& o) const -> float { return x * o.x + y * o.y + z * o.z; }
        [[nodiscard]] auto length() const -> float { return std::sqrt(x * x + y * y + z * z); }
        [[nodiscard]] auto dist(const vector3_t& o) const -> float { return (*this - o).length(); }
    };

    struct vector4_t
    {
        float x{};
        float y{};
        float z{};
        float w{};
    };

    struct matrix3_t
    {
        float data[9]{};
    };

    struct matrix4_t
    {
        float data[16]{};
    };

    struct cframe_t
    {
        matrix3_t rotation{};
        vector3_t position{};
    };

    [[nodiscard]] inline auto w2s(const vector3_t& world, const matrix4_t& vm, const vector2_t& screen) -> vector2_t
    {
        vector4_t clip{};
        clip.x = world.x * vm.data[0] + world.y * vm.data[1] + world.z * vm.data[2] + vm.data[3];
        clip.y = world.x * vm.data[4] + world.y * vm.data[5] + world.z * vm.data[6] + vm.data[7];
        clip.z = world.x * vm.data[8]  + world.y * vm.data[9]  + world.z * vm.data[10] + vm.data[11];
        clip.w = world.x * vm.data[12] + world.y * vm.data[13] + world.z * vm.data[14] + vm.data[15];

        if (clip.w < 0.1f)
            return { -1.f, -1.f };

        const auto inv = 1.f / clip.w;
        const vector3_t ndc{ clip.x * inv, clip.y * inv, clip.z * inv };

        return
        {
            (screen.x * 0.5f * ndc.x) + (ndc.x + screen.x * 0.5f),
            -(screen.y * 0.5f * ndc.y) + (ndc.y + screen.y * 0.5f)
        };
    }
}
