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
#include "sdk/game/shared/netmessages.h"
#include "utils.hpp"

#include <const.h>
#include <engine2/serversideclientbase.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>

class CVoiceFloodFix final : public CFix
{
public:
    CVoiceFloodFix();

    const char* GetName() const override { return "voice_flood"; }

    bool Load(const FixModules& modules, char* error, size_t maxlen) override;
    void Unload() override;

    void OnStartupServer(const GameSessionConfiguration_t& config, const char* pszMapName) override;

public:
    KHook::Return<bool> CServerSideClient_ProcessVoiceData(CServerSideClientBase* pThis, const CNetMessagePB<CCLCMsg_VoiceData>& msg);

    KHook::Virtual<CServerSideClientBase, bool, const CNetMessagePB<CCLCMsg_VoiceData>&>* m_hProcessVoiceData = nullptr;

private:
    using Clock = std::chrono::steady_clock;

    struct Work
    {
        Clock::time_point time{};
        size_t cost = 0;
        uint32_t packets = 0;
    };

    // A new client in a slot inherits at most one second of the old one's budget, so disconnects need no reset.
    struct Budget
    {
        std::deque<Work> recent;
        size_t cost = 0;
        uint32_t packets = 0;
    };

    bool ShouldDrop(CServerSideClientBase* pClient, const CCLCMsg_VoiceData& msg);

    VirtualTable m_VTable;

    Budget m_Budgets[ABSOLUTE_PLAYER_LIMIT];
    uint64_t m_nDropped = 0;
    bool m_bLoggedFirstDrop = false;
};
