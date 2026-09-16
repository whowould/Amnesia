#include "aimbot.hpp"

#include "../overlay/overlay.hpp"
#include "../sdk/cache.hpp"
#include "../sdk/math.hpp"
#include "../sdk/mem.hpp"
#include "../sdk/offsets.hpp"
#include "../sdk/settings.hpp"

#include "imgui.h"

#include <algorithm>
#include <cfloat>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>
#include <thread>

namespace amnesia::features
{
    namespace
    {
        auto now_sec() -> double
        {
            using clock = std::chrono::steady_clock;
            return std::chrono::duration<double>(clock::now().time_since_epoch()).count();
        }

        auto rand_unit() -> float
        {
            return (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) * 2.f - 1.f;
        }

        auto r15_to_r6(std::string_view r15) -> const char*
        {
            if (r15 == "UpperTorso" || r15 == "LowerTorso")
                return "Torso";
            if (r15 == "LeftUpperArm" || r15 == "LeftLowerArm" || r15 == "LeftHand")
                return "Left Arm";
            if (r15 == "RightUpperArm" || r15 == "RightLowerArm" || r15 == "RightHand")
                return "Right Arm";
            if (r15 == "LeftUpperLeg" || r15 == "LeftLowerLeg" || r15 == "LeftFoot")
                return "Left Leg";
            if (r15 == "RightUpperLeg" || r15 == "RightLowerLeg" || r15 == "RightFoot")
                return "Right Leg";
            return nullptr;
        }

        auto r15_to_bb(std::string_view r15) -> const char*
        {
            if (r15 == "Head")
                return "Head";
            if (r15 == "UpperTorso" || r15 == "Chest")
                return "Chest";
            if (r15 == "LowerTorso" || r15 == "HumanoidRootPart" || r15 == "Abdomen")
                return "Abdomen";
            if (r15 == "LeftUpperArm" || r15 == "LeftLowerArm" || r15 == "LeftHand")
                return "LeftArm";
            if (r15 == "RightUpperArm" || r15 == "RightLowerArm" || r15 == "RightHand")
                return "RightArm";
            if (r15 == "LeftUpperLeg" || r15 == "LeftLowerLeg" || r15 == "LeftFoot")
                return "LeftLeg";
            if (r15 == "RightUpperLeg" || r15 == "RightLowerLeg" || r15 == "RightFoot")
                return "RightLeg";
            return nullptr;
        }

        auto is_bb_rig(const player_t& player) -> bool
        {
            for (const auto& part : player.parts)
            {
                if (part.name == "Chest" || part.name == "Abdomen")
                    return true;
            }
            return false;
        }

        auto is_r6_rig(const player_t& player) -> bool
        {
            for (const auto& part : player.parts)
            {
                if (part.name == "Torso")
                    return true;
            }
            return false;
        }

        auto find_named_part(const player_t& player, std::string_view name) -> const part_t*
        {
            for (const auto& part : player.parts)
            {
                if (part.name == name)
                    return &part;
            }
            return nullptr;
        }

        auto resolve_part(const player_t& player, std::string_view wanted) -> const part_t*
        {
            if (const auto* exact = find_named_part(player, wanted))
                return exact;

            if (is_bb_rig(player))
            {
                if (const auto* mapped = r15_to_bb(wanted))
                {
                    if (const auto* part = find_named_part(player, mapped))
                        return part;
                }
            }

            if (is_r6_rig(player))
            {
                if (const auto* mapped = r15_to_r6(wanted))
                {
                    if (const auto* part = find_named_part(player, mapped))
                        return part;
                }
            }

            return nullptr;
        }

        auto posed(const part_t& part) -> bool
        {
            const auto& p = part.cframe.position;
            return p.x != 0.f || p.y != 0.f || p.z != 0.f;
        }

        auto on_screen(const vector2_t& sp, const vector2_t& screen) -> bool
        {
            return sp.x >= 1.f && sp.y >= 1.f && sp.x <= screen.x && sp.y <= screen.y;
        }

        auto dist2d(const vector2_t& a, const vector2_t& b) -> float
        {
            const auto dx = a.x - b.x;
            const auto dy = a.y - b.y;
            return std::sqrt(dx * dx + dy * dy);
        }

        auto apply_humanize(vector3_t world, const std::string& key) -> vector3_t
        {
                if (!settings::aimbot::humanize || settings::aimbot::humanize_radius <= 0.f)
                return world;

            struct state_t
            {
                vector3_t offset{};
                double last_roll{};
                std::string key;
            };
            static state_t state{};

            const auto now = now_sec();
            if (state.key != key || now - state.last_roll >= settings::aimbot::humanize_interval)
            {
                const auto radius = settings::aimbot::humanize_radius;
                state.key = key;
                state.offset = { rand_unit() * radius, rand_unit() * radius, rand_unit() * radius };
                state.last_roll = now;
            }

            return world + state.offset;
        }

        auto mouse_sensitivity() -> float
        {
            auto& memory = mem::get();
            const auto pointer = Offsets::MouseService::SensitivityPointer;
            if (!pointer)
                return 1.f;

            const auto sens_ptr = memory.read<std::uintptr_t>(memory.base() + pointer);
            if (!mem::is_valid(sens_ptr))
                return 1.f;

            const auto value = memory.read<float>(sens_ptr);
            if (value > 0.001f && value < 100.f)
                return value;
            return 1.f;
        }

        auto move_mouse(float dx, float dy) -> void
        {
            INPUT input{};
            input.type = INPUT_MOUSE;
            input.mi.dwFlags = MOUSEEVENTF_MOVE;
            input.mi.dx = static_cast<LONG>(dx);
            input.mi.dy = static_cast<LONG>(dy);
            ::SendInput(1, &input, sizeof(INPUT));
        }

        auto cursor_in_viewport(HWND hwnd) -> POINT
        {
            POINT cursor{};
            ::GetCursorPos(&cursor);
            if (hwnd)
                ::ScreenToClient(hwnd, &cursor);
            return cursor;
        }

        auto roblox_focused() -> bool
        {
            const auto fg = ::GetForegroundWindow();
            if (!fg)
                return false;

            char title[256]{};
            ::GetWindowTextA(fg, title, sizeof(title));
            return std::strstr(title, "Roblox") != nullptr;
        }

        auto key_down() -> bool
        {
            const auto key = settings::aimbot::keys[std::clamp(settings::aimbot::key_index, 0, 11)];
            return key && (::GetAsyncKeyState(key) & 0x8000);
        }

        auto update_key(bool& toggle_last) -> bool
        {
                if (settings::aimbot::key_mode == 0)
            {
                const auto pressed = key_down();
                static bool active = false;
                if (pressed && !toggle_last)
                    active = !active;
                toggle_last = pressed;
                return active;
            }
            if (settings::aimbot::key_mode == 2)
                return true;
            return key_down();
        }

        struct candidate_t
        {
            vector2_t screen{};
            vector3_t world{};
            float distance{ FLT_MAX };
            bool valid{};
        };

        auto sample_part(const player_t& player, std::string_view wanted, const snapshot_t& shot, const vector2_t& cursor, bool skip_fov) -> candidate_t
        {
                candidate_t out{};

            const auto* part = resolve_part(player, wanted);
            if (!part || !posed(*part))
                return out;

            auto world = part->cframe.position;
            if (settings::aimbot::distance_check && world.dist(shot.camera_pos) > settings::aimbot::distance)
                return out;

            if (settings::aimbot::prediction)
            {
                world.x += part->velocity.x * settings::aimbot::prediction_x * 0.01f;
                world.y += part->velocity.y * settings::aimbot::prediction_y * 0.01f;
                world.z += part->velocity.z * settings::aimbot::prediction_x * 0.01f;
            }

            world = apply_humanize(world, player.name + "::" + part->name);

            const auto screen = w2s(world, shot.viewmatrix, shot.dimensions);
            const auto visible = on_screen(screen, shot.dimensions);
            if (!skip_fov && !visible)
                return out;

            const auto dist = visible ? dist2d(screen, cursor) : 0.f;
            if (!skip_fov && settings::aimbot::fov_enabled && dist > settings::aimbot::fov_size)
                return out;

            out.screen = screen;
            out.world = world;
            out.distance = dist;
            out.valid = true;
            return out;
        }


        auto pick_player(const player_t& player, const snapshot_t& shot, const vector2_t& cursor, bool skip_fov) -> candidate_t
        {
                if (settings::aimbot::targeting_mode == 0)
            {
                const auto idx = std::clamp(settings::aimbot::hitbox, 0, 15);
                return sample_part(player, settings::aimbot::hitbox_names[idx], shot, cursor, skip_fov);
            }

            candidate_t best{};
            for (auto i = 0; i < 16; ++i)
            {
                if (!settings::aimbot::hitboxes[i])
                    continue;
                auto hit = sample_part(player, settings::aimbot::hitbox_names[i], shot, cursor, skip_fov);
                if (!hit.valid)
                    continue;
                if (hit.distance < best.distance)
                    best = hit;
            }
            return best;
        }

        auto passes_player(const player_t& player, const snapshot_t& shot) -> bool
        {
                if (player.player == shot.local_player)
                return false;
            if (!player.character)
                return false;
            if (player.health <= 0.f)
                return false;
            if (settings::aimbot::team_check)
            {
                if (player.teammate)
                    return false;
                if (shot.local_team && mem::is_valid(player.player))
                {
                    const auto their_team = mem::get().read<std::uintptr_t>(player.player + Offsets::Player::Team);
                    if (their_team && their_team == shot.local_team)
                        return false;
                }
            }
            if (settings::aimbot::ko_check && player.knocked)
                return false;
            return true;
        }

        auto look_at(const vector3_t& from, const vector3_t& to) -> matrix3_t
        {
            auto forward = to - from;
            const auto len = forward.length();
            if (len < 0.001f)
                return {};

            forward = forward * (1.f / len);

            vector3_t right{ -forward.z, 0.f, forward.x };
            const auto right_len = std::sqrt(right.x * right.x + right.z * right.z);
            if (right_len > 0.001f)
            {
                right.x /= right_len;
                right.z /= right_len;
            }

            const vector3_t up{
                right.y * forward.z - right.z * forward.y,
                right.z * forward.x - right.x * forward.z,
                right.x * forward.y - right.y * forward.x
            };

            matrix3_t rot{};
            rot.data[0] = right.x;
            rot.data[1] = up.x;
            rot.data[2] = -forward.x;
            rot.data[3] = right.y;
            rot.data[4] = up.y;
            rot.data[5] = -forward.y;
            rot.data[6] = right.z;
            rot.data[7] = up.z;
            rot.data[8] = -forward.z;
            return rot;
        }

        auto lerp_matrix(const matrix3_t& from, const matrix3_t& to, float t) -> matrix3_t
        {
            matrix3_t out{};
            for (auto i = 0; i < 9; ++i)
                out.data[i] = from.data[i] + (to.data[i] - from.data[i]) * t;
            return out;
        }

        auto aim_camera(const snapshot_t& shot, const vector3_t& world) -> void
        {
                if (!mem::is_valid(shot.camera))
                return;

            auto rot = look_at(shot.camera_pos, world);
            if (settings::aimbot::smoothing)
            {
                const auto t = 1.f / (std::max)(settings::aimbot::smoothing_x, 1.f);
                rot = lerp_matrix(shot.camera_rot, rot, t);
            }
            mem::get().write<matrix3_t>(shot.camera + Offsets::Camera::Rotation, rot);
        }

        auto aim_mouse(const vector2_t& screen, const POINT& cursor) -> void
        {
                auto dx = screen.x - static_cast<float>(cursor.x);
            auto dy = screen.y - static_cast<float>(cursor.y);
            const auto sensitivity = mouse_sensitivity();
            dx /= sensitivity;
            dy /= sensitivity;
            if (settings::aimbot::smoothing)
            {
                dx /= (std::max)(settings::aimbot::smoothing_x, 1.f);
                dy /= (std::max)(settings::aimbot::smoothing_y, 1.f);
            }
            move_mouse(dx, dy);
        }
    }


    auto run_aimbot(std::stop_token st) -> void
    {
        auto toggle_last = false;
        auto sticky_player = std::uintptr_t{};
        auto dropped = false;

        while (!st.stop_requested())
        {
            if (!roblox_focused())
            {
                sticky_player = 0;
                dropped = false;
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            const auto active = settings::aimbot::enabled && update_key(toggle_last);
            if (!active)
            {
                sticky_player = 0;
                dropped = false;
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            auto shot = cache::get().snapshot();
            if (shot.dimensions.x < 2.f || shot.dimensions.y < 2.f)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(8));
                continue;
            }

            const auto hwnd = overlay::roblox_window();
            const auto cursor = cursor_in_viewport(hwnd);
            const vector2_t cursor_v{ static_cast<float>(cursor.x), static_cast<float>(cursor.y) };

            candidate_t best{};
            auto target_player = std::uintptr_t{};

            if (settings::aimbot::sticky && sticky_player)
            {
                for (const auto& player : shot.players)
                {
                    if (player.player != sticky_player)
                        continue;
                    if (!passes_player(player, shot))
                        break;
                    auto hit = pick_player(player, shot, cursor_v, true);
                    if (hit.valid)
                    {
                        best = hit;
                        target_player = player.player;
                    }
                    break;
                }

                if (!target_player)
                {
                    sticky_player = 0;
                    dropped = true;
                }
            }

            if (!best.valid && !dropped)
            {
                sticky_player = 0;
                for (const auto& player : shot.players)
                {
                    if (!passes_player(player, shot))
                        continue;
                    auto hit = pick_player(player, shot, cursor_v, false);
                    if (!hit.valid)
                        continue;
                    if (hit.distance < best.distance)
                    {
                        best = hit;
                        target_player = player.player;
                    }
                }
                if (best.valid && settings::aimbot::sticky)
                    sticky_player = target_player;
            }

            if (!best.valid)
            {
                sticky_player = 0;
                dropped = false;
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }

            dropped = false;
            if (settings::aimbot::aim_mode == 1)
                aim_mouse(best.screen, cursor);
            else
                aim_camera(shot, best.world);

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    auto draw_aimbot() -> void
    {
        if (!settings::aimbot::fov_show || !settings::aimbot::enabled)
            return;

        const auto hwnd = overlay::roblox_window();
        const auto cursor = cursor_in_viewport(hwnd);
        const ImVec2 center{ static_cast<float>(cursor.x), static_cast<float>(cursor.y) };
        auto* dl = ImGui::GetBackgroundDrawList();
        if (!dl)
            return;

        const auto radius = settings::aimbot::fov_size;
        if (radius <= 0.f)
            return;

        if (settings::aimbot::fov_filled)
            dl->AddCircleFilled(center, radius, IM_COL32(255, 138, 196, 28), 96);
        dl->AddCircle(center, radius, IM_COL32(0, 0, 0, 180), 96, settings::aimbot::fov_thickness + 1.4f);
        dl->AddCircle(center, radius, IM_COL32(255, 138, 196, 220), 96, settings::aimbot::fov_thickness);
    }
}


