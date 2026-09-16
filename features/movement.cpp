#include "movement.hpp"

#include "../sdk/cache.hpp"
#include "../sdk/mem.hpp"
#include "../sdk/offsets.hpp"
#include "../sdk/settings.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <thread>

namespace amnesia::features
{
    namespace
    {
        struct saved_float
        {
            float value{};
            bool has{};
        };

        auto restore(std::uintptr_t addr, saved_float& saved) -> void
        {
            if (!saved.has || !mem::is_valid(addr))
                return;
            mem::get().write<float>(addr, saved.value);
            saved.has = false;
        }

        auto key_down() -> bool
        {
            const auto key = settings::movement::keys[std::clamp(settings::movement::key_index, 0, 11)];
            return key && (::GetAsyncKeyState(key) & 0x8000);
        }

        auto update_key(bool& toggle_last) -> bool
        {
            if (settings::movement::key_mode == 0)
            {
                const auto pressed = key_down();
                static bool active = false;
                if (pressed && !toggle_last)
                    active = !active;
                toggle_last = pressed;
                return active;
            }
            if (settings::movement::key_mode == 2)
                return true;
            return key_down();
        }
    }

    auto run_movement(std::stop_token st) -> void
    {
        auto toggle_last = false;
        auto cached_char = std::uintptr_t{};
        auto cached_hum = std::uintptr_t{};
        saved_float walkspeed{};
        saved_float walkspeed_check{};

        while (!st.stop_requested())
        {
            const auto active = settings::movement::walkspeed && update_key(toggle_last);
            const auto shot = cache::get().snapshot();
            auto character = shot.local_character;
            if (!mem::is_valid(character))
                character = 0;

            if (character != cached_char || !mem::is_valid(cached_hum))
            {
                if (mem::is_valid(cached_hum))
                {
                    restore(cached_hum + Offsets::Humanoid::Walkspeed, walkspeed);
                    restore(cached_hum + Offsets::Humanoid::WalkspeedCheck, walkspeed_check);
                }
                cached_char = character;
                cached_hum = mem::is_valid(character) ? mem::get().find_child_class(character, "Humanoid") : 0;
                walkspeed.has = false;
                walkspeed_check.has = false;
            }

            if (!active)
            {
                if (mem::is_valid(cached_hum))
                {
                    restore(cached_hum + Offsets::Humanoid::Walkspeed, walkspeed);
                    restore(cached_hum + Offsets::Humanoid::WalkspeedCheck, walkspeed_check);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(25));
                continue;
            }

            if (!mem::is_valid(cached_hum))
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(25));
                continue;
            }

            auto& memory = mem::get();
            if (!walkspeed.has)
            {
                walkspeed.value = memory.read<float>(cached_hum + Offsets::Humanoid::Walkspeed);
                walkspeed.has = true;
            }
            if (!walkspeed_check.has)
            {
                walkspeed_check.value = memory.read<float>(cached_hum + Offsets::Humanoid::WalkspeedCheck);
                walkspeed_check.has = true;
            }

            const auto value = settings::movement::walkspeed_value;
            for (auto i = 0; i < 32; ++i)
            {
                memory.write<float>(cached_hum + Offsets::Humanoid::Walkspeed, value);
                memory.write<float>(cached_hum + Offsets::Humanoid::WalkspeedCheck, value);
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(8));
        }

        if (mem::is_valid(cached_hum))
        {
            restore(cached_hum + Offsets::Humanoid::Walkspeed, walkspeed);
            restore(cached_hum + Offsets::Humanoid::WalkspeedCheck, walkspeed_check);
        }
    }
}
