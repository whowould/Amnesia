#include "instance.hpp"

namespace amnesia
{
    auto instance::name() const -> std::string
    {
        return mem::get().get_name(addr_);
    }

    auto instance::classname() const -> std::string
    {
        return mem::get().get_class(addr_);
    }

    auto instance::parent() const -> instance
    {
        return instance{ read<std::uintptr_t>(Offsets::Instance::Parent) };
    }

    auto instance::children() const -> std::vector<instance>
    {
        const auto raw = mem::get().get_children(addr_);
        std::vector<instance> out;
        out.reserve(raw.size());
        for (const auto child : raw)
            out.emplace_back(child);
        return out;
    }

    auto instance::find(std::string_view name) const -> instance
    {
        return instance{ mem::get().find_child(addr_, name) };
    }

    auto instance::find_class(std::string_view classname) const -> instance
    {
        return instance{ mem::get().find_child_class(addr_, classname) };
    }
}
