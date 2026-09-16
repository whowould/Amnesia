#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <winternl.h>

namespace amnesia
{
    using nt_read_vm_t = NTSTATUS(NTAPI*)(HANDLE, PVOID, PVOID, SIZE_T, PSIZE_T);
    using nt_write_vm_t = NTSTATUS(NTAPI*)(HANDLE, PVOID, PVOID, SIZE_T, PSIZE_T);
    using nt_alloc_vm_t = NTSTATUS(NTAPI*)(HANDLE, PVOID*, ULONG_PTR, PSIZE_T, ULONG, ULONG);
    using nt_protect_vm_t = NTSTATUS(NTAPI*)(HANDLE, PVOID*, PSIZE_T, ULONG, PULONG);

    class mem
    {
        HANDLE handle_{ nullptr };
        std::uint32_t pid_{ 0 };
        std::uintptr_t base_{ 0 };

        nt_read_vm_t nt_read_{ nullptr };
        nt_write_vm_t nt_write_{ nullptr };
        nt_alloc_vm_t nt_alloc_{ nullptr };
        nt_protect_vm_t nt_protect_{ nullptr };

        struct rbx_string
        {
            union
            {
                char buffer[16];
                std::uintptr_t pointer;
            } data;
            std::uintptr_t length;
            std::uintptr_t capacity;
        };

    public:
        static auto get() -> mem&
        {
            static mem instance;
            return instance;
        }

        mem();
        ~mem();

        mem(const mem&) = delete;
        auto operator=(const mem&) -> mem& = delete;

        [[nodiscard]] auto get_handle() const -> HANDLE { return handle_; }
        [[nodiscard]] auto get_pid() const -> std::uint32_t { return pid_; }
        [[nodiscard]] auto base() const -> std::uintptr_t { return base_; }

        [[nodiscard]] auto attach(const wchar_t* process_name) -> bool;
        auto detach() -> void;

        [[nodiscard]] static constexpr auto is_valid(std::uintptr_t addr) -> bool
        {
            return addr >= 0x10000 && addr <= 0x7FFFFFFEFFFF;
        }

        template<typename T>
        [[nodiscard]] auto read(std::uintptr_t addr) const -> T
        {
            T value{};
            if (!addr || !handle_ || !nt_read_)
                return value;
            nt_read_(handle_, reinterpret_cast<PVOID>(addr), &value, sizeof(T), nullptr);
            return value;
        }

        template<typename T>
        auto write(std::uintptr_t addr, const T& value) const -> void
        {
            if (!addr || !handle_ || !nt_write_)
                return;
            nt_write_(handle_, reinterpret_cast<PVOID>(addr), const_cast<T*>(&value), sizeof(T), nullptr);
        }

        [[nodiscard]] auto read_buf(std::uintptr_t addr, void* buf, std::size_t size) const -> bool;
        [[nodiscard]] auto write_buf(std::uintptr_t addr, const void* buf, std::size_t size) const -> bool;

        [[nodiscard]] auto read_string(std::uintptr_t addr) const -> std::string;
        [[nodiscard]] auto read_string_field(std::uintptr_t addr) const -> std::string;
        [[nodiscard]] auto read_name_object(std::uintptr_t object) const -> std::string;

        [[nodiscard]] auto get_children(std::uintptr_t parent) const -> std::vector<std::uintptr_t>;
        [[nodiscard]] auto find_child(std::uintptr_t parent, std::string_view name) const -> std::uintptr_t;
        [[nodiscard]] auto find_child_class(std::uintptr_t parent, std::string_view classname) const -> std::uintptr_t;

        [[nodiscard]] auto get_name(std::uintptr_t inst) const -> std::string;
        [[nodiscard]] auto get_username(std::uintptr_t player) const -> std::string;
        [[nodiscard]] auto get_class(std::uintptr_t inst) const -> std::string;

        [[nodiscard]] auto allocate(std::size_t size) const -> std::uintptr_t;
        [[nodiscard]] auto protect(std::uintptr_t addr, std::size_t size, ULONG new_protect, ULONG* old_protect) const -> bool;
    };
}
