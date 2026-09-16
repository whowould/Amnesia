#include <windows.h>

#include "features/aimbot.hpp"
#include "features/players.hpp"
#include "overlay/overlay.hpp"
#include "sdk/cache.hpp"
#include "sdk/console.hpp"
#include "sdk/mem.hpp"
#include "sdk/offsets.hpp"
#include "sdk/threads.hpp"

#include <chrono>
#include <cstdint>
#include <string>
#include <thread>

using namespace amnesia;

namespace
{
    auto wait_for_roblox() -> bool
    {
        console::info("attach", "waiting for RobloxPlayerBeta.exe");

        auto& memory = mem::get();
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);

        while (std::chrono::steady_clock::now() < deadline)
        {
            if (memory.attach(L"RobloxPlayerBeta.exe"))
                return true;
            std::this_thread::sleep_for(std::chrono::milliseconds(400));
        }
        return false;
    }
}

auto main() -> int
{
    console::init();
    console::banner();

    console::info("offsets", "fetching dump + live client version");
    if (!Offsets::fetch())
    {
        if (Offsets::last_error() == "version mismatch")
        {
            console::fail("offsets", "it appears roblox had an update, product is outdated atm wait for further notice");
            if (!Offsets::LiveVersion.empty() && !Offsets::ClientVersion.empty())
                console::fail("offsets", "live " + Offsets::LiveVersion + " vs dump " + Offsets::ClientVersion);
        }
        else
        {
            console::fail("offsets", Offsets::last_error());
        }
        std::this_thread::sleep_for(std::chrono::seconds(4));
        return 1;
    }

    console::ok("offsets", "loaded " + Offsets::ClientVersion + " (dumper " + Offsets::DumperVersion + ")");
    if (!Offsets::DumpedAt.empty())
        console::info("offsets", "dumped at " + Offsets::DumpedAt);
    console::hex("offsets", "FakeDataModel", Offsets::FakeDataModel::Pointer);
    console::hex("offsets", "VisualEngine", Offsets::VisualEngine::Pointer);

    if (!wait_for_roblox())
    {
        console::fail("attach", "roblox not found — open the client and rerun");
        std::this_thread::sleep_for(std::chrono::seconds(3));
        return 1;
    }

    auto& memory = mem::get();
    console::ok("attach", "attached");
    console::dec("attach", "pid", static_cast<std::int64_t>(memory.get_pid()));
    console::hex("attach", "base", memory.base());

    auto& rbx = cache::get();
    rbx.start();

    console::info("cache", "starting datamodel / services / players / transforms");

    if (!rbx.wait_ready(5000))
        console::warn("cache", "datamodel still settling — join a place");
    else
        console::ok("cache", "datamodel resolved");

    if (!rbx.wait_players(3000))
        console::warn("cache", "no players yet — join a place");

    console::hex("cache", "datamodel", rbx.datamodel());
    console::hex("cache", "visualengine", rbx.visual_engine());
    console::hex("cache", "players", rbx.players());
    console::hex("cache", "workspace", rbx.workspace());
    console::hex("cache", "lighting", rbx.lighting());
    console::hex("cache", "camera", rbx.camera());
    console::hex("cache", "mouse", rbx.mouse());
    console::hex("cache", "localplayer", rbx.snapshot().local_player);

    threads::get().start("players", amnesia::features::log_players);
    threads::get().start("aimbot", amnesia::features::run_aimbot);

    console::ok("boot", "running — INSERT toggles menu");
    overlay::run();

    console::warn("boot", "stopping");
    rbx.stop();
    memory.detach();
    console::ok("boot", "exited");
    return 0;
}
