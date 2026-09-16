#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace amnesia::settings
{
    inline bool menu_open = false;
    inline bool esp_enabled = true;
    inline bool esp_box = true;
    inline bool esp_name = true;
    inline bool esp_health = true;

    inline float menu_x = 40.f;
    inline float menu_y = 40.f;
    inline float menu_w = 320.f;
    inline float menu_h = 460.f;
    inline bool vsync_disable = false;
    inline bool streamproof = false;

    namespace movement
    {
        inline constexpr int keys[12] = {
            VK_RBUTTON, VK_LBUTTON, VK_MBUTTON, VK_SHIFT, VK_MENU, VK_CONTROL,
            'X', 'C', 'V', 'Q', 'E', 'F'
        };

        inline bool walkspeed = false;
        inline float walkspeed_value = 16.f;
        inline int key_index = 3;
        inline int key_mode = 1;
    }

    namespace aimbot
    {
        inline constexpr const char* hitbox_names[16] = {
            "Head", "HumanoidRootPart", "UpperTorso", "LowerTorso",
            "LeftUpperArm", "RightUpperArm", "LeftLowerArm", "RightLowerArm",
            "LeftHand", "RightHand", "LeftUpperLeg", "RightUpperLeg",
            "LeftLowerLeg", "RightLowerLeg", "LeftFoot", "RightFoot"
        };

        inline constexpr int keys[12] = {
            VK_RBUTTON, VK_LBUTTON, VK_MBUTTON, VK_SHIFT, VK_MENU, VK_CONTROL,
            'X', 'C', 'V', 'Q', 'E', 'F'
        };

        inline bool enabled = false;
        inline int aim_mode = 0;
        inline int targeting_mode = 0;
        inline int hitbox = 0;
        inline bool hitboxes[16] = { true };
        inline int key_index = 0;
        inline int key_mode = 1;
        inline bool sticky = true;
        inline bool team_check = true;
        inline bool ko_check = true;
        inline bool distance_check = false;
        inline float distance = 1000.f;
        inline bool smoothing = true;
        inline float smoothing_x = 5.f;
        inline float smoothing_y = 5.f;
        inline bool prediction = false;
        inline float prediction_x = 1.f;
        inline float prediction_y = 1.f;
        inline bool humanize = false;
        inline float humanize_radius = 0.35f;
        inline float humanize_interval = 0.15f;
        inline bool fov_enabled = true;
        inline bool fov_show = true;
        inline bool fov_filled = false;
        inline float fov_size = 120.f;
        inline float fov_thickness = 1.5f;
    }
}

