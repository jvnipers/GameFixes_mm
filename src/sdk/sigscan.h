// From FemboyKZ/mm-utils sdk/sigscan.h.

#pragma once

#include <khook.hpp>

#include <cstddef>
#include <cstdint>

namespace sig
{
    bool GetModuleSection(const void* knownAddress, const char* name, void*& outBase, size_t& outSize);

    // KHook reads bytes under its own detours as the originals. Needs PLUGIN_SAVEVARS().
    inline void* FindSignature(void* base, size_t size, const char* signature)
    {
        if (!base || !signature || !*signature)
            return nullptr;

        // KHook doesn't bound its reads by size: only offer starts where a full match fits.
        size_t sigLen = 1;
        for (const char* p = signature; *p; p++)
        {
            if (*p == ' ')
                sigLen++;
        }
        if (size < sigLen)
            return nullptr;

        return KHook::LookupSignature(base, size - sigLen + 1, signature);
    }

    // As a pointer to its first slot.
    void* FindVirtualTable(const void* knownAddress, const char* className);

    // disp32 at +3 (REX + opcode + ModRM), e.g. 48 8D 0D <disp32>.
    inline void* ResolveRipRelative(void* instruction)
    {
        if (!instruction)
            return nullptr;
        uint8_t* p = static_cast<uint8_t*>(instruction) + 3;
        const int32_t disp = *reinterpret_cast<int32_t*>(p);
        return p + 4 + disp;
    }
}
