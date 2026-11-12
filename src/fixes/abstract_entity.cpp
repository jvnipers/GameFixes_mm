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

#include "abstract_entity.h"

#include "utils.hpp"
#include "vprof.hpp"

#include <entity2/entityclass.h>
#include <entity2/entitykeyvalues.h>
#include <entity2/entitysystem.h>
#include <schemasystem/schemasystem.h>

#include <cstdio>

namespace
{
    bool IsConstructible(const CSchemaClassInfo* pInfo)
    {
        if (pInfo && pInfo->m_pPrimaryBinding)
            pInfo = pInfo->m_pPrimaryBinding;
        if (!pInfo)
            return true;

        constexpr uint32 kMask = SCHEMA_CF1_IS_ABSTRACT | SCHEMA_CF1_CONSTRUCT_ALLOWED | SCHEMA_CF1_CONSTRUCT_DISALLOWED;
        return (pInfo->m_nClassFlags1 & kMask) == SCHEMA_CF1_CONSTRUCT_ALLOWED;
    }
}

CAbstractEntityFix::CAbstractEntityFix() :
    KHOOK_NEW(m_hCreateEntityFromKeyValues, this, &CAbstractEntityFix::CEntitySystem_CreateEntityFromKeyValues, nullptr)
{
}

bool CAbstractEntityFix::Load(const FixModules& modules, char* error, size_t maxlen)
{
    void* pCreate = FindPattern(modules.server, WIN_LINUX("40 56 57 41 56 41 57 48 81 EC 98 00 00 00", "55 48 8D 05 ? ? ? ? 48 89 E5 41 57 4C 8D 7D ? 41 56 49 89 F6"));
    if (!pCreate)
    {
        std::snprintf(error, maxlen, "CEntitySystem::CreateEntityFromKeyValues not found");
        return false;
    }

    m_hCreateEntityFromKeyValues->Configure(pCreate);

    Log("hooked CEntitySystem::CreateEntityFromKeyValues (%p)", pCreate);
    return true;
}

void CAbstractEntityFix::Unload()
{
    delete m_hCreateEntityFromKeyValues;
    m_hCreateEntityFromKeyValues = nullptr;
    m_Logged.clear();
}

KHook::Return<CEntityInstance*> CAbstractEntityFix::CEntitySystem_CreateEntityFromKeyValues(CEntitySystem* pThis, CEntityKeyValues* pKeyValues, uint32 nSpawnGroup)
{
    GF_VPROF("GameFixes::abstract_entity::CreateEntityFromKeyValues");

    if (!pThis || !pKeyValues)
        return { KHook::Action::Ignore, nullptr };

    const char* pszClassname = pKeyValues->GetString("classname", nullptr);
    if (!pszClassname || !*pszClassname)
        return { KHook::Action::Ignore, nullptr };

    CEntityClass* pClass = pThis->FindClassByDesignName(pszClassname);
    if (!pClass || !pClass->m_pClassInfo || IsConstructible(pClass->m_pClassInfo->m_pSchemaBinding))
        return { KHook::Action::Ignore, nullptr };

    if (m_Logged.insert(pszClassname).second)
    {
        const char* pszCPPName = pClass->m_pClassInfo->m_pszCPPClassname;
        Log("refused to create '%s' (%s cannot be constructed; the game would crash on it)", pszClassname, pszCPPName ? pszCPPName : "?");
    }

    return { KHook::Action::Supersede, nullptr };
}
