#include "console.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <chrono>
#include <cstdio>
#include <ctime>
#include <mutex>
#include <string>

namespace amnesia::console
{
    std::mutex io_lock;

    namespace
    {
        constexpr auto k_reset = "\033[0m";
        constexpr auto k_dim = "\033[38;2;90;86;110m";
        constexpr auto k_fg = "\033[38;2;214;208;232m";
        constexpr auto k_mute = "\033[38;2;140;132;168m";
        constexpr auto k_ok = "\033[38;2;120;220;170m";
        constexpr auto k_fail = "\033[38;2;255;92;120m";
        constexpr auto k_warn = "\033[38;2;255;196;92m";
        constexpr auto k_accent = "\033[38;2;186;140;255m";
        constexpr auto k_brand = "\033[38;2;255;138;196m";

        auto stamp() -> std::string
        {
            const auto now = std::chrono::system_clock::now();
            const auto tt = std::chrono::system_clock::to_time_t(now);
            std::tm tm{};
            localtime_s(&tm, &tt);

            char buf[16]{};
            std::strftime(buf, sizeof(buf), "%H:%M:%S", &tm);
            return buf;
        }

        auto prefix(std::string_view channel) -> std::string
        {
            std::string out;
            out.reserve(96);
            out += k_dim;
            out += "[ ";
            out += k_mute;
            out += stamp();
            out += k_dim;
            out += " ] [ ";
            out += k_brand;
            out += "amnesia";
            out += k_dim;
            out += " / ";
            out += k_accent;
            out += channel;
            out += k_dim;
            out += " ] ";
            return out;
        }

        auto emit(std::string_view channel, std::string_view color, std::string_view text) -> void
        {
            std::lock_guard lock(io_lock);
            std::fputs(prefix(channel).c_str(), stdout);
            std::fputs(color.data(), stdout);
            std::fwrite(text.data(), 1, text.size(), stdout);
            std::fputs(k_reset, stdout);
            std::fputc('\n', stdout);
            std::fflush(stdout);
        }
    }

    auto init() -> void
    {
        const auto out = ::GetStdHandle(STD_OUTPUT_HANDLE);
        if (out && out != INVALID_HANDLE_VALUE)
        {
            DWORD mode = 0;
            if (::GetConsoleMode(out, &mode))
                ::SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT);
        }
        ::SetConsoleTitleW(L"amnesia");
        ::SetConsoleOutputCP(CP_UTF8);
    }

    auto banner() -> void
    {
        std::lock_guard lock(io_lock);
        std::fputs("\n", stdout);
        std::fputs("  \033[38;2;255;138;196m ▄▄▄       ███▄ ▄███▓ ███▄    █ ▓█████   ██████  ██▓ ▄▄▄\033[0m\n", stdout);
        std::fputs("  \033[38;2;232;118;186m▒████▄    ▓██▒▀█▀ ██▒ ██ ▀█   █ ▓█   ▀ ▒██    ▒ ▓██▒▒████▄\033[0m\n", stdout);
        std::fputs("  \033[38;2;206;110;210m▒██  ▀█▄  ▓██    ▓██░▓██  ▀█ ██▒▒███   ░ ▓██▄   ▒██▒▒██  ▀█▄\033[0m\n", stdout);
        std::fputs("  \033[38;2;176;120;230m░██▄▄▄▄██ ▒██    ▒██ ▓██▒  ▐▌██▒▒▓█  ▄   ▒   ██▒░██░░██▄▄▄▄██\033[0m\n", stdout);
        std::fputs("  \033[38;2;150;132;245m ▓█   ▓██▒▒██▒   ░██▒▒██░   ▓██░░▒████▒▒██████▒▒░██░ ▓█   ▓██▒\033[0m\n", stdout);
        std::fputs("  \033[38;2;120;140;255m ▒▒   ▓▒█░░ ▒░   ░  ░░ ▒░   ▒ ▒ ░░ ▒░ ░▒ ▒▓▒ ▒ ░░▓   ▒▒   ▓▒█░\033[0m\n", stdout);
        std::fputs("\n", stdout);
        std::fputs("  \033[38;2;90;86;110m────────────────────────────────────────────────────────────\033[0m\n", stdout);
        std::fputs("  \033[38;2;140;132;168mexternal  ·  update offsets when roblox updates\033[0m\n", stdout);
        std::fputs("  \033[38;2;90;86;110m────────────────────────────────────────────────────────────\033[0m\n\n", stdout);
        std::fflush(stdout);
    }

    auto info(std::string_view channel, std::string_view text) -> void { emit(channel, k_fg, text); }
    auto ok(std::string_view channel, std::string_view text) -> void { emit(channel, k_ok, text); }
    auto fail(std::string_view channel, std::string_view text) -> void { emit(channel, k_fail, text); }
    auto warn(std::string_view channel, std::string_view text) -> void { emit(channel, k_warn, text); }

    auto hex(std::string_view channel, std::string_view label, std::uintptr_t value) -> void
    {
        char line[192]{};
        std::snprintf(line, sizeof(line), "%.*s -> 0x%llX",
            static_cast<int>(label.size()), label.data(),
            static_cast<unsigned long long>(value));
        emit(channel, k_fg, line);
    }

    auto dec(std::string_view channel, std::string_view label, std::int64_t value) -> void
    {
        char line[192]{};
        std::snprintf(line, sizeof(line), "%.*s -> %lld",
            static_cast<int>(label.size()), label.data(),
            static_cast<long long>(value));
        emit(channel, k_fg, line);
    }
}
