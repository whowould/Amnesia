#include "menu.hpp"

#include "../sdk/settings.hpp"

#include "imgui.h"

namespace amnesia::features
{
    auto draw_menu() -> void
    {
        if (!settings::menu_open)
            return;

        ImGui::SetNextWindowSize(ImVec2(320.f, 460.f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(40.f, 40.f), ImGuiCond_FirstUseEver);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14.f, 12.f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.07f, 0.06f, 0.10f, 0.94f));
        ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.10f, 0.08f, 0.14f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.14f, 0.10f, 0.18f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(1.f, 0.54f, 0.77f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.14f, 0.12f, 0.18f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.35f, 0.22f, 0.40f, 0.6f));

        if (ImGui::Begin("amnesia", &settings::menu_open, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextUnformatted("visuals");
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::Checkbox("ESP", &settings::esp_enabled);
            ImGui::BeginDisabled(!settings::esp_enabled);
            ImGui::Checkbox("box", &settings::esp_box);
            ImGui::Checkbox("name", &settings::esp_name);
            ImGui::Checkbox("health", &settings::esp_health);
            ImGui::EndDisabled();
            ImGui::Spacing();
            ImGui::TextUnformatted("overlay");
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::Checkbox("disable vsync", &settings::vsync_disable);
            ImGui::Checkbox("streamproof", &settings::streamproof);
            ImGui::Spacing();
            ImGui::TextUnformatted("movement");
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::Checkbox("walkspeed", &settings::movement::walkspeed);
            ImGui::BeginDisabled(!settings::movement::walkspeed);
            ImGui::SliderFloat("speed", &settings::movement::walkspeed_value, 0.f, 500.f, "%.0f");
            ImGui::Combo("ws key", &settings::movement::key_index, "RMB\0LMB\0MMB\0Shift\0Alt\0Ctrl\0X\0C\0V\0Q\0E\0F\0");
            ImGui::Combo("ws activation", &settings::movement::key_mode, "toggle\0hold\0always\0");
            ImGui::EndDisabled();
            ImGui::Spacing();
            ImGui::TextUnformatted("aimbot");
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::Checkbox("enabled", &settings::aimbot::enabled);
            ImGui::BeginDisabled(!settings::aimbot::enabled);
            ImGui::Combo("mode", &settings::aimbot::aim_mode, "camera\0mouse\0");
            ImGui::Combo("key", &settings::aimbot::key_index, "RMB\0LMB\0MMB\0Shift\0Alt\0Ctrl\0X\0C\0V\0Q\0E\0F\0");
            ImGui::Combo("activation", &settings::aimbot::key_mode, "toggle\0hold\0always\0");
            ImGui::Combo("target", &settings::aimbot::targeting_mode, "hitbox\0closest part\0");
            if (settings::aimbot::targeting_mode == 0)
                ImGui::Combo("hitbox", &settings::aimbot::hitbox, "Head\0HumanoidRootPart\0UpperTorso\0LowerTorso\0LeftUpperArm\0RightUpperArm\0LeftLowerArm\0RightLowerArm\0LeftHand\0RightHand\0LeftUpperLeg\0RightUpperLeg\0LeftLowerLeg\0RightLowerLeg\0LeftFoot\0RightFoot\0");
            else
            {
                ImGui::TextDisabled("parts");
                for (int i = 0; i < 16; ++i)
                    ImGui::Checkbox(settings::aimbot::hitbox_names[i], &settings::aimbot::hitboxes[i]);
            }
            ImGui::Checkbox("sticky", &settings::aimbot::sticky);
            ImGui::Checkbox("team check", &settings::aimbot::team_check);
            ImGui::Checkbox("ko check", &settings::aimbot::ko_check);
            ImGui::Checkbox("distance check", &settings::aimbot::distance_check);
            ImGui::SliderFloat("distance", &settings::aimbot::distance, 50.f, 2000.f, "%.0f");
            ImGui::Checkbox("smoothing", &settings::aimbot::smoothing);
            ImGui::SliderFloat("smooth x", &settings::aimbot::smoothing_x, 1.f, 20.f, "%.1f");
            ImGui::SliderFloat("smooth y", &settings::aimbot::smoothing_y, 1.f, 20.f, "%.1f");
            ImGui::Checkbox("prediction", &settings::aimbot::prediction);
            ImGui::SliderFloat("pred x", &settings::aimbot::prediction_x, 0.f, 10.f, "%.2f");
            ImGui::SliderFloat("pred y", &settings::aimbot::prediction_y, 0.f, 10.f, "%.2f");
            ImGui::Checkbox("humanize", &settings::aimbot::humanize);
            ImGui::SliderFloat("humanize radius", &settings::aimbot::humanize_radius, 0.f, 2.f, "%.2f");
            ImGui::Checkbox("use fov", &settings::aimbot::fov_enabled);
            ImGui::Checkbox("show fov", &settings::aimbot::fov_show);
            ImGui::Checkbox("fill fov", &settings::aimbot::fov_filled);
            ImGui::SliderFloat("fov", &settings::aimbot::fov_size, 20.f, 600.f, "%.0f");
            ImGui::EndDisabled();
            ImGui::Spacing();
            ImGui::TextDisabled("insert to toggle  ·  esc to quit");

            const auto pos = ImGui::GetWindowPos();
            const auto size = ImGui::GetWindowSize();
            settings::menu_x = pos.x;
            settings::menu_y = pos.y;
            settings::menu_w = size.x;
            settings::menu_h = size.y;
        }
        ImGui::End();

        ImGui::PopStyleColor(6);
        ImGui::PopStyleVar(3);
    }
}
