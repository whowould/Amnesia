#pragma once

#include "math.hpp"

#include <atomic>
#include <cstdint>
#include <memory>
#include <shared_mutex>
#include <stop_token>
#include <string>
#include <unordered_map>
#include <vector>

namespace amnesia
{
    struct part_t
    {
        std::string name;
        std::uintptr_t part{};
        std::uintptr_t primitive{};
        cframe_t cframe{};
        vector3_t size{};
        vector3_t velocity{};
    };

    struct player_t
    {
        std::string name;
        std::string display_name;
        std::uintptr_t player{};
        std::uintptr_t character{};
        std::uintptr_t humanoid{};
        float health{};
        float max_health{};
        float hip_height{ 2.f };
        bool teammate{};
        bool knocked{};
        std::vector<part_t> parts;
    };

    struct snapshot_t
    {
        std::vector<player_t> players;
        std::uintptr_t local_player{};
        std::uintptr_t local_character{};
        std::uintptr_t local_hrp{};
        std::uintptr_t local_prim{};
        std::uintptr_t local_team{};
        std::uintptr_t camera{};
        vector3_t camera_pos{};
        matrix3_t camera_rot{};
        vector2_t dimensions{};
        matrix4_t viewmatrix{};
    };

    class cache
    {
        struct entity_data
        {
            std::vector<player_t> players;
            std::uintptr_t local_player{};
            vector2_t dimensions{};
        };

        struct transform_data
        {
            std::unordered_map<std::string, std::unordered_map<std::uintptr_t, part_t>> parts;
            std::uintptr_t local_character{};
            std::uintptr_t local_hrp{};
            std::uintptr_t local_prim{};
            std::uintptr_t local_team{};
            std::uintptr_t camera{};
            vector3_t camera_pos{};
            matrix3_t camera_rot{};
            vector2_t dimensions{};
            matrix4_t viewmatrix{};
        };

        mutable std::shared_mutex entity_mtx_;
        mutable std::shared_mutex transform_mtx_;
        std::shared_ptr<const entity_data> entities_{ std::make_shared<entity_data>() };
        std::shared_ptr<const transform_data> transforms_{ std::make_shared<transform_data>() };

        std::atomic<std::uintptr_t> base_{ 0 };
        std::atomic<std::uintptr_t> fake_dm_{ 0 };
        std::atomic<std::uintptr_t> datamodel_{ 0 };
        std::atomic<std::uintptr_t> visual_engine_{ 0 };

        std::atomic<std::uintptr_t> players_{ 0 };
        std::atomic<std::uintptr_t> workspace_{ 0 };
        std::atomic<std::uintptr_t> lighting_{ 0 };
        std::atomic<std::uintptr_t> camera_{ 0 };
        std::atomic<std::uintptr_t> mouse_{ 0 };

        std::atomic<bool> running_{ false };

        auto cache_datamodel(std::stop_token st) -> void;
        auto cache_services(std::stop_token st) -> void;
        auto cache_players(std::stop_token st) -> void;
        auto cache_transforms(std::stop_token st) -> void;

        auto publish_entities(entity_data&& next) -> void;
        auto publish_transforms(transform_data&& next) -> void;
        [[nodiscard]] auto take_entities() const -> std::shared_ptr<const entity_data>;

    public:
        static auto get() -> cache&
        {
            static cache instance;
            return instance;
        }

        auto start() -> void;
        auto stop() -> void;
        auto wait_ready(std::uint32_t timeout_ms = 4000) -> bool;
        auto wait_players(std::uint32_t timeout_ms = 4000) -> bool;

        [[nodiscard]] auto base() const -> std::uintptr_t { return base_.load(std::memory_order_acquire); }
        [[nodiscard]] auto datamodel() const -> std::uintptr_t { return datamodel_.load(std::memory_order_acquire); }
        [[nodiscard]] auto visual_engine() const -> std::uintptr_t { return visual_engine_.load(std::memory_order_acquire); }
        [[nodiscard]] auto players() const -> std::uintptr_t { return players_.load(std::memory_order_acquire); }
        [[nodiscard]] auto workspace() const -> std::uintptr_t { return workspace_.load(std::memory_order_acquire); }
        [[nodiscard]] auto lighting() const -> std::uintptr_t { return lighting_.load(std::memory_order_acquire); }
        [[nodiscard]] auto camera() const -> std::uintptr_t { return camera_.load(std::memory_order_acquire); }
        [[nodiscard]] auto mouse() const -> std::uintptr_t { return mouse_.load(std::memory_order_acquire); }

        [[nodiscard]] auto snapshot() const -> snapshot_t;
    };
}
