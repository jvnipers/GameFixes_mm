// From FemboyKZ/mm-utils sdk/sigscan.cpp.

#include "sdk/sigscan.h"

#include <cstring>
#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#include <fcntl.h>
#include <link.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace
{
    // Pass the previous hit as from to walk every match.
    const uint8_t* FindBytes(const uint8_t* base, size_t size, const void* needle, size_t needleLen, const uint8_t* from)
    {
        if (!base || needleLen == 0 || size < needleLen)
            return nullptr;

        const uint8_t* end = base + size - needleLen;
        for (const uint8_t* p = from ? from : base; p <= end; p++)
        {
            if (memcmp(p, needle, needleLen) == 0)
                return p;
        }
        return nullptr;
    }
}

namespace sig
{
#ifdef _WIN32
    static const IMAGE_NT_HEADERS* NtHeaders(const void* knownAddress, uint8_t*& outBase)
    {
        HMODULE hModule = nullptr;
        if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                static_cast<LPCSTR>(knownAddress), &hModule) || !hModule)
            return nullptr;

        // HMODULE is the image base.
        outBase = reinterpret_cast<uint8_t*>(hModule);
        const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(outBase);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE)
            return nullptr;
        const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(outBase + dos->e_lfanew);
        return nt->Signature == IMAGE_NT_SIGNATURE ? nt : nullptr;
    }

    bool GetModuleSection(const void* knownAddress, const char* name, void*& outBase, size_t& outSize)
    {
        uint8_t* base = nullptr;
        const IMAGE_NT_HEADERS* nt = NtHeaders(knownAddress, base);
        if (!nt)
            return false;

        const IMAGE_SECTION_HEADER* sections = IMAGE_FIRST_SECTION(nt);
        for (int i = 0; i < nt->FileHeader.NumberOfSections; i++)
        {
            // Section names are a fixed 8 bytes, NUL terminated only when shorter than that.
            char sectionName[IMAGE_SIZEOF_SHORT_NAME + 1] = {};
            memcpy(sectionName, sections[i].Name, IMAGE_SIZEOF_SHORT_NAME);
            if (strcmp(sectionName, name) != 0)
                continue;

            outBase = base + sections[i].VirtualAddress;
            outSize = sections[i].SizeOfRawData;
            return true;
        }
        return false;
    }

    void* FindVirtualTable(const void* knownAddress, const char* className)
    {
        uint8_t* moduleBase = nullptr;
        void *dataBase = nullptr, *rdataBase = nullptr;
        size_t dataSize = 0, rdataSize = 0;
        if (!NtHeaders(knownAddress, moduleBase) || !GetModuleSection(knownAddress, ".data", dataBase, dataSize)
            || !GetModuleSection(knownAddress, ".rdata", rdataBase, rdataSize))
            return nullptr;

        // MSVC RTTI: the slot before a vtable points at a complete object locator, which names the type descriptor by RVA.
        const std::string decorated = std::string(".?AV") + className + "@@";
        const uint8_t* descriptorName = FindBytes(static_cast<const uint8_t*>(dataBase), dataSize, decorated.c_str(), decorated.size() + 1, nullptr);
        if (!descriptorName || descriptorName < static_cast<const uint8_t*>(dataBase) + 0x10)
            return nullptr;

        const uint32_t descriptorRva = static_cast<uint32_t>((descriptorName - 0x10) - moduleBase);

        const auto* rdata = static_cast<const uint8_t*>(rdataBase);
        for (const uint8_t* ref = FindBytes(rdata, rdataSize, &descriptorRva, sizeof(descriptorRva), nullptr); ref;
             ref = FindBytes(rdata, rdataSize, &descriptorRva, sizeof(descriptorRva), ref + 1))
        {
            if (ref < rdata + 0xC)
                continue;
            // Signature is always 1 on x64, and a base offset of 0 picks the primary vtable's locator.
            if (*reinterpret_cast<const int32_t*>(ref - 0xC) != 1 || *reinterpret_cast<const int32_t*>(ref - 0x8) != 0)
                continue;

            const void* locator = ref - 0xC;
            const uint8_t* slot = FindBytes(rdata, rdataSize, &locator, sizeof(locator), nullptr);
            if (slot)
                return const_cast<uint8_t*>(slot + sizeof(void*));
        }
        return nullptr;
    }
#else
    bool GetModuleSection(const void* knownAddress, const char* name, void*& outBase, size_t& outSize)
    {
        Dl_info dlInfo{};
        if (!dladdr(knownAddress, &dlInfo) || !dlInfo.dli_fbase || !dlInfo.dli_fname)
            return false;

        // Section headers survive only in the file on disk, the runtime mapping keeps program headers.
        int fd = open(dlInfo.dli_fname, O_RDONLY);
        if (fd == -1)
            return false;
        struct stat st{};
        if (fstat(fd, &st) != 0)
        {
            close(fd);
            return false;
        }
        void* map = mmap(nullptr, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        close(fd);
        if (map == MAP_FAILED)
            return false;

        bool found = false;
        const auto* image = static_cast<const uint8_t*>(map);
        const auto* ehdr = reinterpret_cast<const ElfW(Ehdr)*>(image);
        if (ehdr->e_shoff && ehdr->e_shnum)
        {
            const auto* shdrs = reinterpret_cast<const ElfW(Shdr)*>(image + ehdr->e_shoff);
            const char* strTab = reinterpret_cast<const char*>(image + shdrs[ehdr->e_shstrndx].sh_offset);
            for (int i = 0; i < ehdr->e_shnum; i++)
            {
                if (strcmp(strTab + shdrs[i].sh_name, name) != 0)
                    continue;

                outBase = const_cast<uint8_t*>(static_cast<const uint8_t*>(dlInfo.dli_fbase) + shdrs[i].sh_addr);
                outSize = shdrs[i].sh_size;
                found = true;
                break;
            }
        }

        munmap(map, st.st_size);
        return found;
    }

    void* FindVirtualTable(const void* knownAddress, const char* className)
    {
        void *rodataBase = nullptr, *relBase = nullptr;
        size_t rodataSize = 0, relSize = 0;
        if (!GetModuleSection(knownAddress, ".rodata", rodataBase, rodataSize) || !GetModuleSection(knownAddress, ".data.rel.ro", relBase, relSize))
            return nullptr;

        // Itanium: a vtable stores [offset-to-top][typeinfo*] before its first slot; typeinfo points at the length-prefixed name.
        const std::string mangled = std::to_string(strlen(className)) + className;
        const uint8_t* typeName = FindBytes(static_cast<const uint8_t*>(rodataBase), rodataSize, mangled.c_str(), mangled.size() + 1, nullptr);
        if (!typeName)
            return nullptr;

        const uint8_t* nameRef = FindBytes(static_cast<const uint8_t*>(relBase), relSize, &typeName, sizeof(typeName), nullptr);
        if (!nameRef || nameRef < static_cast<const uint8_t*>(relBase) + sizeof(void*))
            return nullptr;
        const void* typeInfo = nameRef - sizeof(void*);

        // Relocated vtables land in either half of .data.rel.ro depending on the link.
        for (const char* sectionName : {".data.rel.ro", ".data.rel.ro.local"})
        {
            void* base = nullptr;
            size_t size = 0;
            if (!GetModuleSection(knownAddress, sectionName, base, size))
                continue;

            const auto* bytes = static_cast<const uint8_t*>(base);
            for (const uint8_t* ref = FindBytes(bytes, size, &typeInfo, sizeof(typeInfo), nullptr); ref;
                 ref = FindBytes(bytes, size, &typeInfo, sizeof(typeInfo), ref + 1))
            {
                // An offset-to-top of 0 picks the primary vtable rather than a secondary base's.
                if (ref >= bytes + sizeof(void*) && *reinterpret_cast<const int64_t*>(ref - sizeof(void*)) == 0)
                    return const_cast<uint8_t*>(ref + sizeof(void*));
            }
        }
        return nullptr;
    }
#endif
}
