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

#include "schemasystem_helper.h"

#include "utils.hpp"

#include <entity2/entityclass.h>
#include <entity2/entitysystem.h>
#include <gameentitysystem.h>
#include <networksystem/inetworkserializer.h>
#include <schemasystem/schemasystem.h>

#include <cstring>
#include <unordered_map>
#include <utility>

#define SCHEMA_SERVER_MODULE WIN_LINUX("server.dll", "libserver.so")

static SchemaClassInfoData_t* FindServerClass(const char* pszClassName)
{
    if (!g_pSchemaSystem)
        return nullptr;

    CSchemaSystemTypeScope* pType = g_pSchemaSystem->FindTypeScopeForModule(SCHEMA_SERVER_MODULE);
    return pType ? pType->FindDeclaredClass(pszClassName).Get() : nullptr;
}

static SchemaClassFieldData_t* FindField(SchemaClassInfoData_t* pClassInfo, const char* pszFieldName,
                                         SchemaClassInfoData_t** ppDeclaring = nullptr)
{
    for (; pClassInfo; pClassInfo = pClassInfo->m_nNumBaseClasses ? pClassInfo->m_pBaseClasses[0].m_pClass : nullptr)
    {
        for (uint16 i = 0; i < pClassInfo->m_nNumFields; ++i)
        {
            if (std::strcmp(pClassInfo->m_pFields[i].m_pName, pszFieldName) == 0)
            {
                if (ppDeclaring)
                    *ppDeclaring = pClassInfo;
                return &pClassInfo->m_pFields[i];
            }
        }
    }

    return nullptr;
}

static CNetworkSerializerCodeGenDatabase* GetSerializerDatabase()
{
    if (!GameEntitySystem())
        return nullptr;

    CEntityClass* pClass = GameEntitySystem()->FindClassByName("CBaseEntity");
    if (!pClass || !pClass->m_NetworkSerializerInfo)
        return nullptr;

    return pClass->m_NetworkSerializerInfo->m_pDatabase;
}

int32_t schema::GetOffset(const char* pszClassName, const char* pszFieldName)
{
    SchemaClassInfoData_t* pClassInfo = FindServerClass(pszClassName);
    if (!pClassInfo)
    {
        Warning("schema::GetOffset: class '%s' was not found!\n", pszClassName);
        return -1;
    }

    SchemaClassFieldData_t* pField = FindField(pClassInfo, pszFieldName);
    if (!pField)
    {
        Warning("schema::GetOffset: '%s::%s' was not found!\n", pszClassName, pszFieldName);
        return -1;
    }

    return pField->m_nSingleInheritanceOffset;
}

void* schema::GetMissingFieldStorage()
{
    alignas(64) static unsigned char s_storage[4096] = {};
    return s_storage;
}

int32_t schema::GetChainOffset(const char* pszClassName)
{
    SchemaClassFieldData_t* pField = FindField(FindServerClass(pszClassName), "__m_pChainEntity");
    return pField ? pField->m_nSingleInheritanceOffset : 0;
}

schema::NetworkInfo_t schema::GetNetworkInfo(const char* pszClassName, const char* pszFieldName)
{
    NetworkInfo_t info;

    CNetworkSerializerCodeGenDatabase* pDatabase = GetSerializerDatabase();
    if (!pDatabase)
        return info; // unresolved, asked again on the next NetworkStateChanged()

    info.resolved = true;
    info.offset = GetOffset(pszClassName, pszFieldName);
    if (info.offset < 0)
        return info;

    SchemaClassInfoData_t* pDeclaring = nullptr;
    FindField(FindServerClass(pszClassName), pszFieldName, &pDeclaring);

    int index = pDatabase->m_ClassInfos.Find(pDeclaring->m_pName);
    info.networked = index != pDatabase->m_ClassInfos.InvalidIndex()
                     && pDatabase->m_ClassInfos[index]->FindField(pszFieldName) != nullptr;
    info.chainOffset = GetChainOffset(pszClassName);

    return info;
}

void schema::EntityNetworkStateChanged(CEntityInstance* pEntity, int32_t nOffset)
{
    pEntity->NetworkStateChanged(NetworkStateChangedData(static_cast<uint32>(nOffset)));
}

void schema::ChainNetworkStateChanged(void* pNetworkVarChainer, int32_t nOffset)
{
    // s2sdk's chainer passes it on with its own path index, and drops it mid-construction.
    NetworkStateChangedData data(static_cast<uint32>(nOffset));
    static_cast<CNetworkVarChainer*>(pNetworkVarChainer)->NetworkStateChanged(data);
}

static bool IsInModuleText(const void* p, const void* pModuleAddress)
{
    static const auto s_text = [pModuleAddress] {
        void* base = nullptr;
        size_t size = 0;
        sig::GetModuleSection(pModuleAddress, ".text", base, size);
        return std::pair(static_cast<const uint8_t*>(base), static_cast<const uint8_t*>(base) + size);
    }();

    return p >= s_text.first && p < s_text.second;
}

// NetworkVar_*::NetworkStateChanged opens with cmp dword ptr [reg+0x38], -1 (m_nPathIndex); no other slot does.
static bool ChecksPathIndex(const std::uint8_t* pCode)
{
    // A wider window ran past tiny functions into the next one.
    for (std::size_t i = 0; i + 4 <= 16; ++i)
    {
        const std::uint8_t modrm = pCode[i + 1];
        // mod 01 (disp8), reg 111 (/7 = cmp), rm not 100 (that one takes a SIB byte)
        if (pCode[i] == 0x83 && (modrm & 0xF8) == 0x78 && modrm != 0x7C && pCode[i + 2] == 0x38 && pCode[i + 3] == 0xFF)
            return true;
    }

    return false;
}

static int FindNetworkStateChangedIndex(void** pVtable)
{
    // The vtable ends where its slots stop pointing into code.
    for (int i = 0; i < 256 && IsInModuleText(pVtable[i], pVtable); ++i)
    {
        if (ChecksPathIndex(static_cast<const std::uint8_t*>(pVtable[i])))
            return i;
    }

    return -1;
}

void schema::EmbeddedNetworkStateChanged(void* pObject, int32_t nOffset)
{
    static std::unordered_map<void**, int> s_indices;

    void** pVtable = *static_cast<void***>(pObject);
    auto it = s_indices.find(pVtable);
    if (it == s_indices.end())
        it = s_indices.emplace(pVtable, FindNetworkStateChangedIndex(pVtable)).first;

    if (it->second < 0)
        return;

    using NetworkStateChangedFn = void (*)(void*, NetworkStateChangedData*);

    NetworkStateChangedData data(static_cast<uint32>(nOffset));
    reinterpret_cast<NetworkStateChangedFn>(pVtable[it->second])(pObject, &data);
}
