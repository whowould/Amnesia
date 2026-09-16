#include "mem.hpp"
#include "offsets.hpp"

#include <TlHelp32.h>
#include <cstring>

namespace amnesia
{
    mem::mem()
    {
        const auto ntdll = ::GetModuleHandleW(L"ntdll.dll");
        if (!ntdll)
            return;

        nt_read_ = reinterpret_cast<nt_read_vm_t>(::GetProcAddress(ntdll, "NtReadVirtualMemory"));
        nt_write_ = reinterpret_cast<nt_write_vm_t>(::GetProcAddress(ntdll, "NtWriteVirtualMemory"));
        nt_alloc_ = reinterpret_cast<nt_alloc_vm_t>(::GetProcAddress(ntdll, "NtAllocateVirtualMemory"));
        nt_protect_ = reinterpret_cast<nt_protect_vm_t>(::GetProcAddress(ntdll, "NtProtectVirtualMemory"));
    }

    mem::~mem()
    {
        detach();
    }

    auto mem::detach() -> void
    {
        if (handle_)
        {
            ::CloseHandle(handle_);
            handle_ = nullptr;
        }
        pid_ = 0;
        base_ = 0;
    }

    auto mem::attach(const wchar_t* process_name) -> bool
    {
        detach();

        const auto snap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE)
            return false;

        PROCESSENTRY32W pe{ sizeof(pe) };
        auto found = false;
        if (::Process32FirstW(snap, &pe))
        {
            do
            {
                if (::wcscmp(pe.szExeFile, process_name) == 0)
                {
                    pid_ = pe.th32ProcessID;
                    found = true;
                    break;
                }
            } while (::Process32NextW(snap, &pe));
        }
        ::CloseHandle(snap);

        if (!found || !pid_)
            return false;

        handle_ = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid_);
        if (!handle_)
            return false;

        const auto mods = ::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid_);
        if (mods == INVALID_HANDLE_VALUE)
            return false;

        MODULEENTRY32W me{ sizeof(me) };
        if (::Module32FirstW(mods, &me))
            base_ = reinterpret_cast<std::uintptr_t>(me.modBaseAddr);
        ::CloseHandle(mods);

        return base_ != 0;
    }

    auto mem::read_buf(std::uintptr_t addr, void* buf, std::size_t size) const -> bool
    {
        if (!addr || !buf || !size || !handle_ || !nt_read_)
            return false;

        SIZE_T got{};
        const auto status = nt_read_(handle_, reinterpret_cast<PVOID>(addr), buf, size, &got);
        return status == 0 && got == size;
    }

    auto mem::write_buf(std::uintptr_t addr, const void* buf, std::size_t size) const -> bool
    {
        if (!addr || !buf || !size || !handle_ || !nt_write_)
            return false;

        SIZE_T put{};
        const auto status = nt_write_(handle_, reinterpret_cast<PVOID>(addr), const_cast<void*>(buf), size, &put);
        return status == 0 && put == size;
    }

    auto mem::read_string(std::uintptr_t addr) const -> std::string
    {
        if (!is_valid(addr))
            return {};

        const auto length_off = Offsets::Misc::StringLength ? Offsets::Misc::StringLength : 0x10;
        auto size = static_cast<int>(read<std::uint64_t>(addr + length_off) & 0xFFFFFFFF);
        if (size <= 0 || size > 255)
            size = read<int>(addr + length_off);
        if (size <= 0 || size > 255)
            return {};

        const auto src = (size >= 16) ? read<std::uintptr_t>(addr) : addr;
        if (!is_valid(src))
            return {};

        char buf[256]{};
        if (!read_buf(src, buf, static_cast<std::size_t>(size)))
            return {};

        return std::string(buf, static_cast<std::size_t>(size));
    }

    auto mem::read_string_field(std::uintptr_t addr) const -> std::string
    {
        if (!is_valid(addr))
            return {};

        auto text = read_string(addr);
        if (!text.empty())
            return text;

        const auto ptr = read<std::uintptr_t>(addr);
        if (!is_valid(ptr) || ptr == addr)
            return {};

        text = read_string(ptr);
        if (!text.empty())
            return text;

        const auto nested = read<std::uintptr_t>(ptr);
        if (is_valid(nested) && nested != ptr)
            return read_string(nested);
        return {};
    }

    auto mem::get_children(std::uintptr_t parent) const -> std::vector<std::uintptr_t>
    {
        std::vector<std::uintptr_t> out;
        if (!is_valid(parent))
            return out;

        const auto child_ptr = read<std::uintptr_t>(parent + Offsets::Instance::ChildrenStart);
        if (!is_valid(child_ptr))
            return out;

        const auto start = read<std::uintptr_t>(child_ptr);
        const auto end = read<std::uintptr_t>(child_ptr + Offsets::Instance::ChildrenEnd);
        if (!is_valid(start) || !is_valid(end) || start >= end)
            return out;

        const auto span = end - start;
        if (span > 160000)
            return out;

        const auto count = span / 16;
        if (count == 0 || count > 10000)
            return out;

        std::vector<std::uint8_t> blob(span);
        if (!read_buf(start, blob.data(), span))
            return out;

        out.reserve(count);
        for (std::size_t i = 0; i < count; ++i)
        {
            std::uintptr_t child{};
            std::memcpy(&child, blob.data() + (i * 16), sizeof(child));
            if (child)
                out.push_back(child);
        }
        return out;
    }

    auto mem::read_name_object(std::uintptr_t object) const -> std::string
    {
        if (!is_valid(object))
            return {};

        auto text = read_string(object);
        if (!text.empty())
            return text;

        const auto inner = Offsets::Instance::Name;
        if (inner && inner <= 0x20)
        {
            text = read_string(object + inner);
            if (!text.empty())
                return text;
            text = read_string_field(object + inner);
            if (!text.empty())
                return text;
        }

        return read_string_field(object);
    }

    auto mem::get_name(std::uintptr_t inst) const -> std::string
    {
        if (!is_valid(inst))
            return {};

        if (Offsets::Instance::NameContainer)
        {
            const auto container = read<std::uintptr_t>(inst + Offsets::Instance::NameContainer);
            auto name = read_name_object(container);
            if (!name.empty())
                return name;
        }

        if (Offsets::Instance::Name && Offsets::Instance::Name > 0x20)
        {
            auto name = read_name_object(read<std::uintptr_t>(inst + Offsets::Instance::Name));
            if (!name.empty())
                return name;
            name = read_string_field(inst + Offsets::Instance::Name);
            if (!name.empty())
                return name;
        }

        return {};
    }

    auto mem::get_username(std::uintptr_t player) const -> std::string
    {
        return get_name(player);
    }

    auto mem::get_class(std::uintptr_t inst) const -> std::string
    {
        if (!is_valid(inst))
            return {};
        const auto desc = read<std::uintptr_t>(inst + Offsets::Instance::ClassDescriptor);
        if (!is_valid(desc))
            return {};
        const auto class_ptr = read<std::uintptr_t>(desc + Offsets::Instance::ClassName);
        return class_ptr ? read_string(class_ptr) : std::string{};
    }

    auto mem::find_child(std::uintptr_t parent, std::string_view name) const -> std::uintptr_t
    {
        for (const auto child : get_children(parent))
        {
            if (get_name(child) == name)
                return child;
        }
        return 0;
    }

    auto mem::find_child_class(std::uintptr_t parent, std::string_view classname) const -> std::uintptr_t
    {
        for (const auto child : get_children(parent))
        {
            if (get_class(child) == classname)
                return child;
        }
        return 0;
    }

    auto mem::allocate(std::size_t size) const -> std::uintptr_t
    {
        if (!handle_ || !nt_alloc_ || !size)
            return 0;

        PVOID allocated = nullptr;
        SIZE_T region = size;
        const auto status = nt_alloc_(handle_, &allocated, 0, &region, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        if (status != 0)
            return 0;
        return reinterpret_cast<std::uintptr_t>(allocated);
    }

    auto mem::protect(std::uintptr_t addr, std::size_t size, ULONG new_protect, ULONG* old_protect) const -> bool
    {
        if (!handle_ || !nt_protect_ || !is_valid(addr) || !size)
            return false;

        PVOID base = reinterpret_cast<PVOID>(addr);
        SIZE_T region = size;
        ULONG  old = 0;
        const auto status = nt_protect_(handle_, &base, &region, new_protect, &old);
        if (old_protect)
            *old_protect = old;
        return status == 0;
    }
}
