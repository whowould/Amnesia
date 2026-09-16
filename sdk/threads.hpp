#pragma once

#include <functional>
#include <stop_token>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace amnesia
{
    class threads
    {
        std::vector<std::jthread> workers_;

    public:
        static auto get() -> threads&
        {
            static threads instance;
            return instance;
        }

        auto start(std::string_view name, std::function<void(std::stop_token)> fn) -> void;
        auto stop() -> void;

        threads() = default;
        ~threads() { stop(); }

        threads(const threads&) = delete;
        auto operator=(const threads&) -> threads& = delete;
    };
}
