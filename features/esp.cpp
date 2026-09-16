#include "esp.hpp"

#include "../overlay/overlay.hpp"
#include "../sdk/cache.hpp"
#include "../sdk/math.hpp"
#include "../sdk/settings.hpp"

#include "imgui.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <string>
#include <string_view>

namespace amnesia::features
{
    namespace
    {
        auto project(const vector3_t& world, const matrix4_t& vm, const vector2_t& screen, vector2_t& out) -> bool
        {
            vector4_t clip{};
            clip.x = world.x * vm.data[0] + world.y * vm.data[1] + world.z * vm.data[2] + vm.data[3];
            clip.y = world.x * vm.data[4] + world.y * vm.data[5] + world.z * vm.data[6] + vm.data[7];
            clip.z = world.x * vm.data[8] + world.y * vm.data[9] + world.z * vm.data[10] + vm.data[11];
            clip.w = world.x * vm.data[12] + world.y * vm.data[13] + world.z * vm.data[14] + vm.data[15];
            if (clip.w < 0.1f)
                return false;

            const auto inv = 1.f / clip.w;
            out.x = (screen.x * 0.5f) * (clip.x * inv + 1.f);
            out.y = (screen.y * 0.5f) * (1.f - clip.y * inv);
            return std::isfinite(out.x) && std::isfinite(out.y);
        }

        auto outlined_text(ImDrawList* dl, ImFont* font, float size, ImVec2 pos, ImU32 color, const char* text) -> void
        {
            pos.x = std::floor(pos.x + 0.5f);
            pos.y = std::floor(pos.y + 0.5f);
            const ImU32 outline = IM_COL32(0, 0, 0, 230);
            for (int ox = -1; ox <= 1; ++ox)
            {
                for (int oy = -1; oy <= 1; ++oy)
                {
                    if (ox == 0 && oy == 0)
                        continue;
                    dl->AddText(font, size, ImVec2(pos.x + static_cast<float>(ox), pos.y + static_cast<float>(oy)), outline, text);
                }
            }
            dl->AddText(font, size, pos, color, text);
        }

        auto draw_name(ImDrawList* dl, ImVec2 center_top, const player_t& player) -> void
        {
            auto* font = overlay::name_font();
            if (!font)
                font = ImGui::GetFont();
            if (!font)
                return;

            const auto size = font->LegacySize > 1.f ? font->LegacySize : 13.f;
            const auto user = "[" + player.name + "]";
            const auto display = player.display_name.empty() ? player.name : player.display_name;

            const auto user_w = font->CalcTextSizeA(size, FLT_MAX, 0.f, user.c_str()).x;
            const auto gap = font->CalcTextSizeA(size, FLT_MAX, 0.f, "  ").x;
            const auto display_w = font->CalcTextSizeA(size, FLT_MAX, 0.f, display.c_str()).x;
            const auto total = user_w + gap + display_w;
            const auto line_h = font->CalcTextSizeA(size, FLT_MAX, 0.f, user.c_str()).y;

            ImVec2 pos{ center_top.x - total * 0.5f, center_top.y - line_h - 3.f };
            outlined_text(dl, font, size, pos, IM_COL32(255, 138, 196, 255), user.c_str());
            pos.x += user_w + gap;
            outlined_text(dl, font, size, pos, IM_COL32(248, 236, 246, 255), display.c_str());
        }

        auto is_accessory(std::string_view name) -> bool
        {
            return name.rfind("Acc:", 0) == 0;
        }

        auto is_root(std::string_view name) -> bool
        {
            return name == "HumanoidRootPart";
        }

        auto is_r6_body(std::string_view name) -> bool
        {
            return name == "Head" || name == "Torso"
                || name == "Left Arm" || name == "Right Arm"
                || name == "Left Leg" || name == "Right Leg";
        }

        auto is_r15_body(std::string_view name) -> bool
        {
            return name == "Head" || name == "UpperTorso" || name == "LowerTorso"
                || name == "LeftUpperArm" || name == "LeftLowerArm" || name == "LeftHand"
                || name == "RightUpperArm" || name == "RightLowerArm" || name == "RightHand"
                || name == "LeftUpperLeg" || name == "LeftLowerLeg" || name == "LeftFoot"
                || name == "RightUpperLeg" || name == "RightLowerLeg" || name == "RightFoot";
        }

        auto is_bb_body(std::string_view name) -> bool
        {
            return name == "Head" || name == "Chest" || name == "Abdomen"
                || name == "LeftArm" || name == "RightArm"
                || name == "LeftLeg" || name == "RightLeg";
        }

        auto has_named(const player_t& player, std::string_view name) -> bool
        {
            for (const auto& part : player.parts)
            {
                if (part.name == name)
                    return true;
            }
            return false;
        }

        auto box_part(const player_t& player, std::string_view name) -> bool
        {
            if (is_accessory(name) || is_root(name))
                return false;
            if (has_named(player, "Torso") && !has_named(player, "UpperTorso"))
                return is_r6_body(name);
            if (has_named(player, "UpperTorso") || has_named(player, "LowerTorso"))
                return is_r15_body(name);
            if (has_named(player, "Chest") || has_named(player, "Abdomen"))
                return is_bb_body(name);
            return is_r6_body(name) || is_r15_body(name) || is_bb_body(name);
        }

        auto find_part(const player_t& player, std::string_view name) -> const part_t*
        {
            for (const auto& part : player.parts)
            {
                if (part.name == name)
                    return &part;
            }
            return nullptr;
        }

        auto posed(const part_t& part) -> bool
        {
            const auto& p = part.cframe.position;
            return p.x != 0.f || p.y != 0.f || p.z != 0.f;
        }

        auto rotate(const matrix3_t& rot, const vector3_t& local) -> vector3_t
        {
            return {
                rot.data[0] * local.x + rot.data[1] * local.y + rot.data[2] * local.z,
                rot.data[3] * local.x + rot.data[4] * local.y + rot.data[5] * local.z,
                rot.data[6] * local.x + rot.data[7] * local.y + rot.data[8] * local.z,
            };
        }

        auto expand_obb(const part_t& part, const matrix4_t& vm, const vector2_t& screen, vector2_t& bmin, vector2_t& bmax) -> int
        {
            const auto& pos = part.cframe.position;
            const auto& rot = part.cframe.rotation;
            auto size = part.size;
            if (size.x < 0.05f && size.y < 0.05f && size.z < 0.05f)
                size = { 1.f, 1.f, 1.f };

            const vector3_t half{ size.x * 0.5f, size.y * 0.5f, size.z * 0.5f };
            const vector3_t local[8] = {
                { -half.x, -half.y, -half.z }, { -half.x, -half.y,  half.z },
                { -half.x,  half.y, -half.z }, { -half.x,  half.y,  half.z },
                {  half.x, -half.y, -half.z }, {  half.x, -half.y,  half.z },
                {  half.x,  half.y, -half.z }, {  half.x,  half.y,  half.z },
            };

            auto hits = 0;
            for (const auto& c : local)
            {
                const auto world = pos + rotate(rot, c);
                vector2_t sc{};
                if (!project(world, vm, screen, sc))
                    continue;
                ++hits;
                bmin.x = (std::min)(bmin.x, sc.x);
                bmin.y = (std::min)(bmin.y, sc.y);
                bmax.x = (std::max)(bmax.x, sc.x);
                bmax.y = (std::max)(bmax.y, sc.y);
            }
            return hits;
        }

        auto expand_head(const vector3_t& pos, const matrix4_t& vm, const vector2_t& screen, vector2_t& bmin, vector2_t& bmax) -> int
        {
            constexpr auto rx = 0.62f;
            constexpr auto ry = 0.72f;
            constexpr auto rz = 0.62f;
            const vector3_t pts[6] = {
                { pos.x + rx, pos.y, pos.z }, { pos.x - rx, pos.y, pos.z },
                { pos.x, pos.y + ry, pos.z }, { pos.x, pos.y - ry, pos.z },
                { pos.x, pos.y, pos.z + rz }, { pos.x, pos.y, pos.z - rz },
            };

            auto hits = 0;
            for (const auto& world : pts)
            {
                vector2_t sc{};
                if (!project(world, vm, screen, sc))
                    continue;
                ++hits;
                bmin.x = (std::min)(bmin.x, sc.x);
                bmin.y = (std::min)(bmin.y, sc.y);
                bmax.x = (std::max)(bmax.x, sc.x);
                bmax.y = (std::max)(bmax.y, sc.y);
            }
            return hits;
        }
    }

    auto draw_esp() -> void
    {
        if (!settings::esp_enabled)
            return;

        const auto shot = cache::get().snapshot();
        if (shot.players.empty())
            return;

        vector2_t screen = shot.dimensions;
        if (screen.x < 2.f || screen.y < 2.f)
        {
            const auto display = ImGui::GetIO().DisplaySize;
            screen = { display.x, display.y };
        }
        if (screen.x < 2.f || screen.y < 2.f)
            return;

        auto* dl = ImGui::GetBackgroundDrawList();
        const ImU32 box_col = IM_COL32(255, 138, 196, 255);

        for (const auto& player : shot.players)
        {
            if (player.player == shot.local_player)
                continue;
            if (player.parts.empty() || !player.character)
                continue;

            const auto r6 = has_named(player, "Torso") && !has_named(player, "UpperTorso");
            vector2_t bmin{ FLT_MAX, FLT_MAX };
            vector2_t bmax{ -FLT_MAX, -FLT_MAX };
            auto hits = 0;
            auto body_hits = 0;

            for (const auto& part : player.parts)
            {
                if (!box_part(player, part.name))
                    continue;
                if (!posed(part))
                    continue;

                const auto used = (r6 && part.name == "Head")
                    ? expand_head(part.cframe.position, shot.viewmatrix, screen, bmin, bmax)
                    : expand_obb(part, shot.viewmatrix, screen, bmin, bmax);
                if (used <= 0)
                    continue;
                hits += used;
                ++body_hits;
            }

            if (body_hits == 0)
            {
                const part_t* head = find_part(player, "Head");
                const part_t* torso = find_part(player, "UpperTorso");
                if (!torso)
                    torso = find_part(player, "Torso");
                if (!torso)
                    torso = find_part(player, "LowerTorso");
                const part_t* root = find_part(player, "HumanoidRootPart");
                const part_t* pivot = torso ? torso : (root && posed(*root) ? root : nullptr);
                if (!pivot || !posed(*pivot))
                    continue;

                const auto& rot = pivot->cframe.rotation;
                const auto up = rotate(rot, { 0.f, 1.f, 0.f });
                auto hip = player.hip_height;
                if (hip < 0.4f || hip > 12.f)
                    hip = 2.f;

                vector3_t top = pivot->cframe.position;
                if (head && posed(*head))
                {
                    auto head_h = head->size.y;
                    if (head_h < 0.2f)
                        head_h = 1.2f;
                    top = head->cframe.position + up * (head_h * 0.55f);
                }
                else
                {
                    top = pivot->cframe.position + up * 1.6f;
                }

                const auto feet = pivot->cframe.position - up * (hip + 0.35f);
                const auto width_world = (std::max)(1.4f, hip * 0.7f);
                const vector3_t right = rotate(rot, { 1.f, 0.f, 0.f });
                const vector3_t corners[4] = {
                    top + right * (width_world * 0.5f),
                    top - right * (width_world * 0.5f),
                    feet + right * (width_world * 0.5f),
                    feet - right * (width_world * 0.5f),
                };

                for (const auto& world : corners)
                {
                    vector2_t sc{};
                    if (!project(world, shot.viewmatrix, screen, sc))
                        continue;
                    ++hits;
                    bmin.x = (std::min)(bmin.x, sc.x);
                    bmin.y = (std::min)(bmin.y, sc.y);
                    bmax.x = (std::max)(bmax.x, sc.x);
                    bmax.y = (std::max)(bmax.y, sc.y);
                }
            }

            if (hits <= 0 || bmin.x >= bmax.x || bmin.y >= bmax.y)
                continue;

            if (bmax.x < 0.f || bmax.y < 0.f || bmin.x > screen.x || bmin.y > screen.y)
                continue;

            bmin.x = std::floor((std::max)(0.f, bmin.x) + 0.5f);
            bmin.y = std::floor((std::max)(0.f, bmin.y) + 0.5f);
            bmax.x = std::floor((std::min)(screen.x, bmax.x) + 0.5f);
            bmax.y = std::floor((std::min)(screen.y, bmax.y) + 0.5f);

            const auto width = bmax.x - bmin.x;
            const auto height = bmax.y - bmin.y;
            if (width < 4.f || height < 8.f)
                continue;
            if (width > screen.x * 0.85f && height > screen.y * 0.85f)
                continue;

            if (settings::esp_box)
            {
                const ImU32 outline = IM_COL32(0, 0, 0, 220);
                auto stroke = [&](float x0, float y0, float x1, float y1, ImU32 col)
                {
                    dl->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y0 + 1.f), col);
                    dl->AddRectFilled(ImVec2(x0, y1 - 1.f), ImVec2(x1, y1), col);
                    dl->AddRectFilled(ImVec2(x0, y0), ImVec2(x0 + 1.f, y1), col);
                    dl->AddRectFilled(ImVec2(x1 - 1.f, y0), ImVec2(x1, y1), col);
                };
                stroke(bmin.x - 1.f, bmin.y - 1.f, bmax.x + 1.f, bmax.y + 1.f, outline);
                stroke(bmin.x, bmin.y, bmax.x, bmax.y, box_col);
                stroke(bmin.x + 1.f, bmin.y + 1.f, bmax.x - 1.f, bmax.y - 1.f, outline);
            }

            if (settings::esp_health && player.max_health > 0.f)
            {
                const auto frac = (std::clamp)(player.health / player.max_health, 0.f, 1.f);
                const auto x0 = bmin.x - 7.f;
                const auto x1 = bmin.x - 3.f;
                const auto y0 = bmin.y - 1.f;
                const auto y1 = bmax.y + 1.f;
                auto fill_top = bmax.y - std::floor(height * frac + 0.5f);
                if (fill_top < bmin.y)
                    fill_top = bmin.y;

                dl->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y1), IM_COL32(0, 0, 0, 220));
                if (frac > 0.f)
                {
                    const auto r = static_cast<int>((1.f - frac) * 255.f);
                    const auto g = static_cast<int>(frac * 255.f);
                    const ImU32 top = IM_COL32(r, g, 80, 255);
                    const ImU32 bot = IM_COL32(r * 2 / 5, g * 2 / 5, 32, 255);
                    dl->AddRectFilledMultiColor(
                        ImVec2(x0 + 1.f, fill_top),
                        ImVec2(x1 - 1.f, y1 - 1.f),
                        top, top, bot, bot);
                }
            }

            if (settings::esp_name && !player.name.empty())
                draw_name(dl, ImVec2((bmin.x + bmax.x) * 0.5f, bmin.y), player);
        }
    }
}
