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

#include "steam_ban.h"

#include "utils.hpp"
#include "vprof.hpp"

#include <steam/steamclientpublic.h>
#include <tier0/convar.h>
#include <tier0/keyvalues.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <vector>

CSteamBanFix::CSteamBanFix() :
    KHOOK_NEW(m_hCheckSteamBan, this, &CSteamBanFix::GameSystem_Think_CheckSteamBan, &CSteamBanFix::GameSystem_Think_CheckSteamBanPost)
{
}

void CSteamBanFix::ReadConfig(KeyValues* pConfig)
{
    m_bClearAfterPass = pConfig->GetBool("clear_after_pass", true);
    m_holdTime = std::chrono::duration<float>((std::max)(pConfig->GetFloat("hold_seconds", 15.0f), 0.0f));

    KeyValues* pWhitelist = pConfig->FindKey("whitelist");
    if (!pWhitelist)
        return;

    for (KeyValues* pEntry = pWhitelist->GetFirstSubKey(); pEntry; pEntry = pEntry->GetNextKey())
    {
        const char* pszSteamId64 = pEntry->GetString(nullptr, "");
        const CSteamID steamId((std::strtoull(pszSteamId64, nullptr, 10)));

        if (!steamId.IsValid())
        {
            Log("ignoring whitelist entry \"%s\" \"%s\": not a SteamID64", pEntry->GetName(), pszSteamId64);
            continue;
        }

        m_whitelist.insert(steamId.GetAccountID());
    }
}

bool CSteamBanFix::Load(const FixModules& modules, char* error, size_t maxlen)
{
    // https://github.com/Source2ZE/CS2Fixes/commit/c82d21ae36588520391b301ed02bfb851dff18e1

    // lea reg, [rip + CCSGameRules::sm_mapGcBanInformation]
    void* pMapRef = FindPattern(modules.server, WIN_LINUX("48 8D 0D ? ? ? ? 45 33 C0 4C 89 74 24", "48 8D 35 ? ? ? ? 89 C2 48 63 46"));
    if (!pMapRef)
    {
        std::snprintf(error, maxlen, "CCSGameRules::sm_mapGcBanInformation not found");
        return false;
    }

    m_pBanMap = static_cast<decltype(m_pBanMap)>(sig::ResolveRipRelative(pMapRef));

    void* pCheckSteamBan = FindPattern(modules.server, WIN_LINUX("41 54 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 44 8B 60 58", "55 48 89 E5 41 57 41 56 41 55 41 54 53 48 83 EC ? 48 8B 05 ? ? ? ? 44 8B 68 ? 45 85 ED 0F 8E ? ? ? ? 8B 15"));
    if (!pCheckSteamBan)
    {
        std::snprintf(error, maxlen, "GameSystem_Think_CheckSteamBan not found");
        return false;
    }

    m_hCheckSteamBan->Configure(pCheckSteamBan);

    Log("hooked GameSystem_Think_CheckSteamBan (%p), ban map at %p, %zu whitelisted account(s), clear after pass %s, hold %.0f s", pCheckSteamBan, m_pBanMap, m_whitelist.size(), m_bClearAfterPass ? "on" : "off", m_holdTime.count());
    return true;
}

void CSteamBanFix::Unload()
{
    delete m_hCheckSteamBan;
    m_hCheckSteamBan = nullptr;
    m_pBanMap = nullptr;
    m_firstSeen.clear();
}

bool CSteamBanFix::AllowAccount(uint64_t steamId64)
{
    const CSteamID steamId(static_cast<uint64>(steamId64));
    if (!steamId.IsValid())
        return false;

    m_allowed.insert(steamId.GetAccountID());
    return true;
}

bool CSteamBanFix::DisallowAccount(uint64_t steamId64)
{
    const CSteamID steamId(static_cast<uint64>(steamId64));
    return steamId.IsValid() && m_allowed.erase(steamId.GetAccountID()) > 0;
}

KHook::Return<void> CSteamBanFix::GameSystem_Think_CheckSteamBan()
{
    GF_VPROF("GameFixes::steam_ban::CheckSteamBan");

    m_bPassSkipped = false;

    if (!m_pBanMap || m_pBanMap->Count() <= 0)
    {
        m_firstSeen.clear();
        return { KHook::Action::Ignore };
    }

    // Below sv_kick_players_with_cooldown 2 the pass still acts on competitive cooldowns (reasons 20, 22 and 23).
    static ConVarRefAbstract sv_kick_players_with_cooldown("sv_kick_players_with_cooldown");
    const bool bDropCooldowns = sv_kick_players_with_cooldown.IsValidRef() && sv_kick_players_with_cooldown.GetInt() < 2;

    // Collect first, remove after -- mutating the tree mid-walk isn't safe.
    std::vector<int> toRemove;
    FOR_EACH_MAP(*m_pBanMap, i)
    {
        const uint32 uReason = m_pBanMap->Element(i).m_uiReason;

        if (m_whitelist.contains(m_pBanMap->Key(i)) || m_allowed.contains(m_pBanMap->Key(i)))
            toRemove.push_back(i);
        else if (bDropCooldowns && (uReason == 20 || uReason == 22 || uReason == 23))
            toRemove.push_back(i);
    }

    for (int i : toRemove)
        m_pBanMap->RemoveAt(i);

    if (m_holdTime.count() <= 0.0f)
        return { KHook::Action::Ignore };

    // Hold what would be kicked while it's new, in case a plugin allows it in time.
    const auto now = std::chrono::steady_clock::now();
    bool bHold = false;
    std::unordered_set<uint32> inMap;
    FOR_EACH_MAP(*m_pBanMap, i)
    {
        const uint32 uAccount = m_pBanMap->Key(i);
        inMap.insert(uAccount);
        const auto firstSeen = m_firstSeen.try_emplace(uAccount, now).first->second;
        if (now - firstSeen < m_holdTime)
            bHold = true;
    }
    std::erase_if(m_firstSeen, [&inMap](const auto& entry) { return !inMap.contains(entry.first); });

    if (!bHold)
        return { KHook::Action::Ignore };

    m_bPassSkipped = true;
    return { KHook::Action::Supersede };
}

KHook::Return<void> CSteamBanFix::GameSystem_Think_CheckSteamBanPost()
{
    GF_VPROF("GameFixes::steam_ban::CheckSteamBanPost");

    if (m_bPassSkipped)
    {
        m_bPassSkipped = false;
        return { KHook::Action::Ignore };
    }

    // The rest is stale and must not reach the next player to join (shared by @aiolos1045).
    if (m_bClearAfterPass && m_pBanMap && m_pBanMap->Count() > 0)
        m_pBanMap->RemoveAll();

    return { KHook::Action::Ignore };
}
