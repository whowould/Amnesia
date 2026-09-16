#include "cache.hpp"
#include "instance.hpp"
#include "mem.hpp"
#include "offsets.hpp"
#include "threads.hpp"

#include <chrono>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <utility>

namespace amnesia
{
    namespace
    {
        using clock = std::chrono::steady_clock;

        auto sleep_for(std::stop_token st, std::chrono::milliseconds ms) -> bool
        {
            const auto until = clock::now() + ms;
            while (clock::now() < until)
            {
                if (st.stop_requested())
                    return false;
                std::this_thread::sleep_for(std::chrono::milliseconds(4));
            }
            return true;
        }

        auto is_part_class(std::string_view cls) -> bool
        {
            return cls.find("Part") != std::string_view::npos;
        }

        auto is_model_class(std::string_view cls) -> bool
        {
            return cls == "Model" || cls == "Actor";
        }

        auto unwrap_instance(mem& memory, std::uintptr_t ptr) -> std::uintptr_t
        {
            if (!mem::is_valid(ptr))
                return 0;

            const auto cls = memory.get_class(ptr);
            if (!cls.empty())
                return ptr;

            const auto direct = memory.read<std::uintptr_t>(ptr);
            if (mem::is_valid(direct) && !memory.get_class(direct).empty())
                return direct;

            const auto inner = memory.read<std::uintptr_t>(ptr + 0x8);
            if (mem::is_valid(inner) && !memory.get_class(inner).empty())
                return inner;

            return ptr;
        }

        auto resolve_character(mem& memory, std::uintptr_t player, std::uintptr_t workspace, std::string_view player_name) -> std::uintptr_t
        {
            const auto raw = memory.read<std::uintptr_t>(player + Offsets::Player::ModelInstance);
            auto character = unwrap_instance(memory, raw);
            if (mem::is_valid(character) && memory.find_child_class(character, "Humanoid"))
                return character;

            if (mem::is_valid(workspace) && !player_name.empty())
            {
                const auto by_name = memory.find_child(workspace, player_name);
                if (mem::is_valid(by_name) && memory.find_child_class(by_name, "Humanoid"))
                    return by_name;
            }

            if (mem::is_valid(character))
                return character;
            return mem::is_valid(raw) ? raw : 0;
        }

        auto push_part(mem& memory, std::vector<part_t>& out, std::uintptr_t inst, std::string name) -> void
        {
            if (!mem::is_valid(inst))
                return;

            for (const auto& existing : out)
            {
                if (existing.part == inst)
                    return;
            }

            part_t body{};
            body.name = std::move(name);
            if (body.name.empty())
                body.name = memory.get_name(inst);
            body.part = inst;
            body.primitive = memory.read<std::uintptr_t>(inst + Offsets::BasePart::Primitive);
            out.push_back(std::move(body));
        }

        auto collect_parts(mem& memory, std::uintptr_t character, std::unordered_map<std::uintptr_t, std::string>& class_cache) -> std::vector<part_t>
        {
            std::vector<part_t> out;
            const auto kids = memory.get_children(character);
            out.reserve(kids.size() + 16);

            for (const auto part : kids)
            {
                if (!mem::is_valid(part))
                    continue;

                const auto desc = memory.read<std::uintptr_t>(part + Offsets::Instance::ClassDescriptor);
                if (!desc)
                    continue;

                std::string cls;
                if (const auto hit = class_cache.find(desc); hit != class_cache.end())
                    cls = hit->second;
                else
                {
                    cls = memory.get_class(part);
                    class_cache[desc] = cls;
                }

                if (cls == "Accessory")
                {
                    const auto handle = memory.find_child(part, "Handle");
                    if (!mem::is_valid(handle))
                        continue;

                    part_t acc{};
                    acc.name = "Acc:" + memory.get_name(part);
                    if (acc.name == "Acc:")
                        acc.name = "Accessory";
                    acc.part = handle;
                    acc.primitive = memory.read<std::uintptr_t>(handle + Offsets::BasePart::Primitive);
                    out.push_back(std::move(acc));
                    continue;
                }

                if (cls == "Humanoid" || cls == "Animator" || cls == "BodyColors" || cls == "Shirt" || cls == "Pants" || cls == "ShirtGraphic" || cls == "Decal" || cls == "Script" || cls == "LocalScript" || cls == "BillboardGui" || cls == "Highlight")
                    continue;

                if (is_model_class(cls))
                    continue;

                if (!is_part_class(cls) && cls != "UnionOperation" && cls != "TrussPart" && cls != "WedgePart" && cls != "CornerWedgePart" && cls != "SpawnLocation" && cls != "Seat" && cls != "VehicleSeat")
                    continue;

                auto name = memory.get_name(part);
                if (name.empty())
                    name = cls;
                push_part(memory, out, part, std::move(name));
            }

            static constexpr std::string_view body_names[] = {
                "Head", "HumanoidRootPart", "Torso", "UpperTorso", "LowerTorso",
                "Left Arm", "Right Arm", "Left Leg", "Right Leg",
                "LeftUpperArm", "LeftLowerArm", "LeftHand",
                "RightUpperArm", "RightLowerArm", "RightHand",
                "LeftUpperLeg", "LeftLowerLeg", "LeftFoot",
                "RightUpperLeg", "RightLowerLeg", "RightFoot",
            };

            for (const auto name : body_names)
            {
                auto have = false;
                for (const auto& part : out)
                {
                    if (part.name == name)
                    {
                        have = true;
                        break;
                    }
                }
                if (have)
                    continue;

                const auto inst = memory.find_child(character, name);
                if (mem::is_valid(inst))
                    push_part(memory, out, inst, std::string{ name });
            }

            return out;
        }

        auto body_ready(const std::vector<part_t>& parts) -> bool
        {
            auto has_head = false;
            auto has_torso = false;
            auto body = 0;
            for (const auto& part : parts)
            {
                if (part.name.rfind("Acc:", 0) == 0)
                    continue;
                ++body;
                if (part.name == "Head")
                    has_head = true;
                if (part.name == "Torso" || part.name == "UpperTorso" || part.name == "LowerTorso")
                    has_torso = true;
            }
            return has_head && has_torso && body >= 6;
        }
    }

    auto cache::publish_entities(entity_data&& next) -> void
    {
        auto boxed = std::make_shared<const entity_data>(std::move(next));
        std::unique_lock lock(entity_mtx_);
        entities_ = std::move(boxed);
    }

    auto cache::publish_transforms(transform_data&& next) -> void
    {
        auto boxed = std::make_shared<const transform_data>(std::move(next));
        std::unique_lock lock(transform_mtx_);
        transforms_ = std::move(boxed);
    }

    auto cache::take_entities() const -> std::shared_ptr<const entity_data>
    {
        std::shared_lock lock(entity_mtx_);
        return entities_;
    }

    auto cache::snapshot() const -> snapshot_t
    {
        std::shared_ptr<const entity_data> entities;
        std::shared_ptr<const transform_data> transforms;
        {
            std::shared_lock lock(entity_mtx_);
            entities = entities_;
        }
        {
            std::shared_lock lock(transform_mtx_);
            transforms = transforms_;
        }

        snapshot_t out{};
        if (entities)
        {
            out.players = entities->players;
            out.local_player = entities->local_player;
            out.dimensions = entities->dimensions;
        }
        if (transforms)
        {
            out.local_character = transforms->local_character;
            out.local_hrp = transforms->local_hrp;
            out.local_prim = transforms->local_prim;
            out.local_team = transforms->local_team;
            out.camera = transforms->camera;
            out.camera_pos = transforms->camera_pos;
            out.camera_rot = transforms->camera_rot;
            out.viewmatrix = transforms->viewmatrix;
            if (transforms->dimensions.x > 0.f)
                out.dimensions = transforms->dimensions;

            for (auto& player : out.players)
            {
                const auto hit = transforms->parts.find(player.name);
                if (hit == transforms->parts.end())
                    continue;

                for (auto& part : player.parts)
                {
                    auto it = hit->second.find(part.part);
                    if (it == hit->second.end() && part.primitive)
                        it = hit->second.find(part.primitive);
                    if (it == hit->second.end())
                        continue;
                    part.cframe = it->second.cframe;
                    part.size = it->second.size;
                    part.velocity = it->second.velocity;
                    if (it->second.primitive)
                        part.primitive = it->second.primitive;
                }
            }
        }
        return out;
    }

    auto cache::start() -> void
    {
        if (running_.exchange(true, std::memory_order_acq_rel))
            return;

        auto& pool = threads::get();
        pool.start("datamodel", [this](std::stop_token st) { cache_datamodel(st); });
        pool.start("services", [this](std::stop_token st) { cache_services(st); });
        pool.start("players", [this](std::stop_token st) { cache_players(st); });
        pool.start("transforms", [this](std::stop_token st) { cache_transforms(st); });
    }

    auto cache::stop() -> void
    {
        threads::get().stop();
        running_.store(false, std::memory_order_release);
    }

    auto cache::wait_ready(std::uint32_t timeout_ms) -> bool
    {
        const auto deadline = clock::now() + std::chrono::milliseconds(timeout_ms);
        while (clock::now() < deadline)
        {
            if (datamodel() && players() && visual_engine())
                return true;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        return datamodel() && players();
    }

    auto cache::wait_players(std::uint32_t timeout_ms) -> bool
    {
        const auto deadline = clock::now() + std::chrono::milliseconds(timeout_ms);
        while (clock::now() < deadline)
        {
            const auto shot = snapshot();
            if (!shot.players.empty())
                return true;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        return !snapshot().players.empty();
    }

    auto cache::cache_datamodel(std::stop_token st) -> void
    {
        auto& memory = mem::get();
        while (!st.stop_requested())
        {
            const auto image = memory.base();
            base_.store(image, std::memory_order_release);

            if (image)
            {
                const auto fake = memory.read<std::uintptr_t>(image + Offsets::FakeDataModel::Pointer);
                fake_dm_.store(fake, std::memory_order_release);

                const auto real = mem::is_valid(fake)
                    ? memory.read<std::uintptr_t>(fake + Offsets::FakeDataModel::RealDataModel)
                    : 0;
                datamodel_.store(real, std::memory_order_release);

                const auto ve = memory.read<std::uintptr_t>(image + Offsets::VisualEngine::Pointer);
                visual_engine_.store(ve, std::memory_order_release);
            }

            if (!sleep_for(st, std::chrono::seconds(2)))
                return;
        }
    }

    auto cache::cache_services(std::stop_token st) -> void
    {
        auto& memory = mem::get();
        while (!st.stop_requested())
        {
            const auto dm = datamodel();
            if (!mem::is_valid(dm))
            {
                if (!sleep_for(st, std::chrono::milliseconds(120)))
                    return;
                continue;
            }

            const instance root{ dm };
            const auto ws = root.find_class("Workspace");
            const auto plr = root.find_class("Players");
            const auto lit = root.find_class("Lighting");
            const auto mse = root.find_class("MouseService");

            workspace_.store(ws.address(), std::memory_order_release);
            players_.store(plr.address(), std::memory_order_release);
            lighting_.store(lit.address(), std::memory_order_release);
            mouse_.store(mse.address(), std::memory_order_release);

            if (ws)
            {
                auto cam = ws.find("Camera");
                if (!cam)
                    cam = instance{ memory.read<std::uintptr_t>(ws.address() + Offsets::Workspace::CurrentCamera) };
                camera_.store(cam.address(), std::memory_order_release);
            }

            if (!sleep_for(st, std::chrono::seconds(2)))
                return;
        }
    }

    auto cache::cache_players(std::stop_token st) -> void
    {
        auto& memory = mem::get();

        struct cached_player
        {
            std::uintptr_t character{};
            std::uintptr_t humanoid{};
            std::uintptr_t player{};
            std::string display_name;
            std::vector<part_t> parts;
        };

        std::unordered_map<std::string, cached_player> last;
        std::unordered_map<std::uintptr_t, std::string> class_cache;
        auto prune_tick = 0;

        while (!st.stop_requested())
        {
            const auto players_s = players();
            const auto visual = visual_engine();
            if (!mem::is_valid(players_s) || !mem::is_valid(visual))
            {
                if (!sleep_for(st, std::chrono::milliseconds(100)))
                    return;
                continue;
            }

            if (++prune_tick >= 300)
            {
                prune_tick = 0;
                class_cache.clear();
            }

            entity_data next{};
            next.local_player = memory.read<std::uintptr_t>(players_s + Offsets::Player::LocalPlayer);
            next.dimensions = memory.read<vector2_t>(visual + Offsets::VisualEngine::Dimensions);

            const auto kids = memory.get_children(players_s);
            next.players.reserve(kids.size());

            std::unordered_map<std::string, cached_player> keep;
            keep.reserve(kids.size());

            for (const auto player : kids)
            {
                if (!mem::is_valid(player))
                    continue;

                if (memory.get_class(player) != "Player")
                    continue;

                auto player_name = memory.get_name(player);
                const auto character = resolve_character(memory, player, workspace(), player_name);
                if (mem::is_valid(character))
                {
                    auto character_name = memory.get_name(character);
                    if (!character_name.empty())
                        player_name = std::move(character_name);
                }
                if (player_name.empty())
                    continue;

                player_t entry{};
                entry.name = player_name;
                entry.player = player;
                entry.character = character;

                auto prev = last.find(player_name);
                const auto character_changed =
                    prev == last.end() || prev->second.character != character || prev->second.player != player;

                auto display = memory.read_string(player + Offsets::Player::DisplayName);
                if (display.empty())
                    display = memory.read_string_field(player + Offsets::Player::DisplayName);
                if (display.empty() && !character_changed)
                    display = prev->second.display_name;
                entry.display_name = std::move(display);

                cached_player slot{};
                slot.character = character;
                slot.player = player;
                slot.display_name = entry.display_name;

                if (mem::is_valid(character))
                {
                    std::uintptr_t humanoid = 0;
                    if (!character_changed && mem::is_valid(prev->second.humanoid))
                        humanoid = prev->second.humanoid;
                    else
                    {
                        humanoid = memory.find_child_class(character, "Humanoid");
                        if (!humanoid)
                            humanoid = memory.find_child(character, "Humanoid");
                    }

                    entry.humanoid = humanoid;
                    slot.humanoid = humanoid;

                    if (mem::is_valid(humanoid))
                    {
                        entry.health = memory.read<float>(humanoid + Offsets::Humanoid::Health);
                        entry.max_health = memory.read<float>(humanoid + Offsets::Humanoid::MaxHealth);
                        entry.hip_height = memory.read<float>(humanoid + Offsets::Humanoid::HipHeight);
                        if (entry.hip_height < 0.4f || entry.hip_height > 12.f)
                            entry.hip_height = 2.f;
                        if (Offsets::Humanoid::DisplayName)
                        {
                            auto nametag = memory.read_string(humanoid + Offsets::Humanoid::DisplayName);
                            if (nametag.empty())
                                nametag = memory.read_string_field(humanoid + Offsets::Humanoid::DisplayName);
                            if (!nametag.empty() && nametag != entry.name)
                                entry.display_name = std::move(nametag);
                            else if (entry.display_name.empty())
                                entry.display_name = std::move(nametag);
                        }
                    }

                    const auto reuse =
                        !character_changed &&
                        !prev->second.parts.empty() &&
                        body_ready(prev->second.parts);

                    if (reuse)
                        entry.parts = prev->second.parts;
                    else
                        entry.parts = collect_parts(memory, character, class_cache);

                    slot.parts = entry.parts;
                }

                keep.emplace(player_name, std::move(slot));
                next.players.push_back(std::move(entry));
            }

            last = std::move(keep);
            publish_entities(std::move(next));

            if (!sleep_for(st, std::chrono::milliseconds(80)))
                return;
        }
    }

    auto cache::cache_transforms(std::stop_token st) -> void
    {
        auto& memory = mem::get();
        auto size_tick = 0;
        std::unordered_map<std::uintptr_t, vector3_t> size_cache;

        std::uintptr_t cached_char = 0;
        std::uintptr_t cached_hrp = 0;
        std::uintptr_t cached_prim = 0;

        while (!st.stop_requested())
        {
            const auto entities = take_entities();
            if (!entities || entities->players.empty())
            {
                if (!sleep_for(st, std::chrono::milliseconds(40)))
                    return;
                continue;
            }

            transform_data next{};
            next.camera = camera();
            if (mem::is_valid(next.camera))
            {
                next.camera_pos = memory.read<vector3_t>(next.camera + Offsets::Camera::Position);
                next.camera_rot = memory.read<matrix3_t>(next.camera + Offsets::Camera::Rotation);
            }

            const auto lp = entities->local_player;
            if (mem::is_valid(lp))
            {
                next.local_character = memory.read<std::uintptr_t>(lp + Offsets::Player::ModelInstance);
                next.local_team = memory.read<std::uintptr_t>(lp + Offsets::Player::Team);

                if (mem::is_valid(next.local_character))
                {
                    if (next.local_character != cached_char || !mem::is_valid(cached_hrp))
                    {
                        cached_char = next.local_character;
                        cached_hrp = memory.find_child(cached_char, "HumanoidRootPart");
                        cached_prim = mem::is_valid(cached_hrp)
                            ? memory.read<std::uintptr_t>(cached_hrp + Offsets::BasePart::Primitive)
                            : 0;
                    }
                    next.local_hrp = cached_hrp;
                    next.local_prim = cached_prim;
                }
            }

            const auto ve = visual_engine();
            if (mem::is_valid(ve))
            {
                next.viewmatrix = memory.read<matrix4_t>(ve + Offsets::VisualEngine::ViewMatrix);
                next.dimensions = memory.read<vector2_t>(ve + Offsets::VisualEngine::Dimensions);
            }

            const auto refresh_sizes = (++size_tick % 12) == 0;

            for (const auto& player : entities->players)
            {
                auto& dest = next.parts[player.name];
                dest.reserve(player.parts.size());

                for (const auto& part : player.parts)
                {
                    auto primitive = part.primitive;
                    if (!mem::is_valid(primitive) && mem::is_valid(part.part))
                        primitive = memory.read<std::uintptr_t>(part.part + Offsets::BasePart::Primitive);
                    if (!mem::is_valid(primitive))
                        continue;

                    part_t posed = part;
                    posed.primitive = primitive;
                    posed.cframe = memory.read<cframe_t>(primitive + Offsets::Primitive::Rotation);
                    posed.velocity = memory.read<vector3_t>(primitive + Offsets::Primitive::AssemblyLinearVelocity);

                    if (refresh_sizes)
                    {
                        posed.size = memory.read<vector3_t>(primitive + Offsets::Primitive::Size);
                        size_cache[primitive] = posed.size;
                    }
                    else if (const auto hit = size_cache.find(primitive); hit != size_cache.end())
                    {
                        posed.size = hit->second;
                    }
                    else
                    {
                        posed.size = memory.read<vector3_t>(primitive + Offsets::Primitive::Size);
                        size_cache[primitive] = posed.size;
                    }

                    dest.emplace(part.part ? part.part : primitive, posed);
                }
            }

            if (size_tick % 180 == 0)
                size_cache.clear();

            publish_transforms(std::move(next));

            if (!sleep_for(st, std::chrono::milliseconds(8)))
                return;
        }
    }
}
