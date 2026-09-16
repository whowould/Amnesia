#include "players.hpp"

#include "../sdk/cache.hpp"
#include "../sdk/console.hpp"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <string>
#include <thread>

namespace amnesia::features
{
    auto log_players(std::stop_token st) -> void
    {
        auto last_count = static_cast<std::size_t>(-1);

        while (!st.stop_requested())
        {
            const auto shot = cache::get().snapshot();
            if (shot.players.size() != last_count)
            {
                last_count = shot.players.size();
                if (last_count == 0)
                    continue;

                console::dec("players", "count", static_cast<std::int64_t>(last_count));
                if (shot.local_player)
                    console::hex("players", "localplayer", shot.local_player);

                for (const auto& player : shot.players)
                {
                    char line[256]{};
                    std::snprintf(line, sizeof(line), "[%s] %s  char 0x%llX  hp %d/%d  parts %zu",
                        player.name.c_str(),
                        player.display_name.empty() ? "-" : player.display_name.c_str(),
                        static_cast<unsigned long long>(player.character),
                        static_cast<int>(player.health),
                        static_cast<int>(player.max_health),
                        player.parts.size());
                    console::info("players", line);
                }
            }

            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
    }
}
