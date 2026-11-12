/**
 * vim: set ts=4 sw=4 tw=99 noet:
 * =============================================================================
 * GameFixes_mm
 * Copyright (C) 2026 Michal "Slynx (˙·٠● S l y n x ●٠·˙)" Přikryl.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *   - Michal "Slynx (˙·٠● S l y n x ●٠·˙)" Přikryl
 *
 * Project: GameFixes_mm
 */

#pragma once

#include "sdk/sigscan.h"

#include <khook/memory.hpp>

#include <cstddef>
#include <cstring>
#include <type_traits>

#ifdef _WIN32
#define WIN_LINUX(win, linux) win
#else
#define WIN_LINUX(win, linux) linux
#endif

inline void* FindPattern(const void* knownAddress, const char* pattern)
{
    void* base = nullptr;
    size_t size = 0;
    return sig::GetModuleSection(knownAddress, ".text", base, size) ? sig::FindSignature(base, size, pattern) : nullptr;
}

inline void WriteCode(void* addr, const void* pBytes, std::size_t len)
{
#ifdef _WIN32
    DWORD oldProtect;
    VirtualProtect(addr, len, PAGE_EXECUTE_READWRITE, &oldProtect);
    std::memcpy(addr, pBytes, len);
    VirtualProtect(addr, len, oldProtect, &oldProtect);
#else
    // Linux has no mprotect() query, so code goes through an RWX -> RX window.
    KHook::Memory::SetAccess(addr, len, KHook::Memory::READ | KHook::Memory::WRITE | KHook::Memory::EXECUTE);
    std::memcpy(addr, pBytes, len);
    KHook::Memory::SetAccess(addr, len, KHook::Memory::READ | KHook::Memory::EXECUTE);
#endif
}

// KHook's AddGlobal / RemoveGlobal only read an object's first pointer: this stands in for one, hooking every object with that vtable.
struct VirtualTable
{
    void** m_pVTFs = nullptr;
};

template<typename T>
inline T* AsHookTarget(VirtualTable& vtable)
{
    return reinterpret_cast<T*>(&vtable);
}

// Takes the hook's type from the member's declaration; MSVC fails to deduce some KHook constructors.
#define KHOOK_NEW(member, ...) member(new std::remove_pointer_t<decltype(member)>(__VA_ARGS__))
