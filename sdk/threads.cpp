#include "threads.hpp"
#include "console.hpp"

#include <utility>

namespace amnesia
{
    auto threads::start(std::string_view name, std::function<void(std::stop_token)> fn) -> void
    {
        const auto label = std::string{ name };
        workers_.emplace_back([label, fn = std::move(fn)](std::stop_token st) mutable
        {
            console::ok("thread", label + " started");
            fn(st);
            console::warn("thread", label + " stopped");
        });
    }

    auto threads::stop() -> void
    {
        for (auto& worker : workers_)
            worker.request_stop();
        workers_.clear();
    }
}
