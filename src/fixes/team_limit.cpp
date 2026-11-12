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

#include "team_limit.h"

#include "sdk/schema/classes/CCSGameRulesProxy.h"

#include "vprof.hpp"

#include <engine2/ivengineserver2.h>
#include <gameentitysystem.h>
#include <interfaces/interfaces.h>
#include <tier0/tier0_strtools.h>

#include <initializer_list>

bool CTeamLimitFix::Load(const FixModules& modules, char* error, size_t maxlen)
{
    return true;
}

void CTeamLimitFix::Unload()
{
    m_pGameRules = nullptr;
}

void CTeamLimitFix::OnStartupServer(const GameSessionConfiguration_t& config, const char* pszMapName)
{
    m_pGameRules = nullptr;
}

void CTeamLimitFix::OnGameFrame(bool simulating)
{
    if (!simulating)
        return;

    GF_VPROF("GameFixes::team_limit::OnGameFrame");

    CCSGameRules* pGameRules = GetGameRules();
    CGlobalVars* pGlobals = g_pEngineServer->GetServerGlobals();
    if (!pGameRules || !pGlobals)
        return;

    // None of these are networked; a plain write is enough
    const int iMaxPlayers = pGlobals->maxClients;
    for (int32_t* pLimit : { &pGameRules->m_iMaxNumTerrorists(), &pGameRules->m_iMaxNumCTs(),
                             &pGameRules->m_iSpawnPointCount_Terrorist(), &pGameRules->m_iSpawnPointCount_CT() })
    {
        if (*pLimit < iMaxPlayers)
            *pLimit = iMaxPlayers;
    }
}

CCSGameRules* CTeamLimitFix::GetGameRules()
{
    if (m_pGameRules)
        return m_pGameRules;

    CGameEntitySystem* pEntitySystem = GameEntitySystem();
    if (!pEntitySystem)
        return nullptr;

    for (CEntityIdentity* pIdentity = pEntitySystem->m_EntityList.m_pFirstActiveEntity; pIdentity; pIdentity = pIdentity->m_pNext)
    {
        if (!pIdentity->m_pInstance || V_strcmp(pIdentity->GetClassname(), "cs_gamerules") != 0)
            continue;

        m_pGameRules = static_cast<CCSGameRulesProxy*>(pIdentity->m_pInstance)->m_pGameRules();
        break;
    }

    return m_pGameRules;
}
