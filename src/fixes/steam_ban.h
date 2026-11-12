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

#include "fix.h"
#include "plugin.h"
#include "sdk/game/server/gcbaninformation.h"

#include <tier0/utlmap.h>

#include <chrono>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>

class CSteamBanFix final : public CFix
{
public:
    CSteamBanFix();

    const char* GetName() const override { return "steam_ban"; }

    void ReadConfig(KeyValues* pConfig) override;
    bool Load(const FixModules& modules, char* error, size_t maxlen) override;
    void Unload() override;

    bool AllowAccount(uint64_t steamId64);
    bool DisallowAccount(uint64_t steamId64);

public:
    KHook::Return<void> GameSystem_Think_CheckSteamBan();
    KHook::Return<void> GameSystem_Think_CheckSteamBanPost();

    KHook::Function<void>* m_hCheckSteamBan = nullptr;

private:
    std::unordered_set<uint32> m_whitelist;

    bool m_bClearAfterPass = true;

    std::unordered_set<uint32> m_allowed;

    std::chrono::duration<float> m_holdTime{15.0f};
    std::unordered_map<uint32, std::chrono::steady_clock::time_point> m_firstSeen;

    bool m_bPassSkipped = false;

    // The uint32 third argument matches the engine's layout: index-based access only, never Find / Insert.
    CUtlOrderedMap<uint32, CGcBanInformation_t, uint32>* m_pBanMap = nullptr;
};
