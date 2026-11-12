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

#include "utils.hpp"

#include <engine2/inetworkserverservice.h>
#include <engine2/isource2server.h>
#include <engine2/ivengineserver2.h>
#include <engine2/networkgameserverbase.h>
#include <igameevents.h>
#include <ISmmPlugin.h>

#include "IGameFixes.h"

#include <memory>
#include <vector>

class CFix;
struct FixModules;

class Plugin final : public ISmmPlugin, public IMetamodListener, public IGameFixes
{
public:
    Plugin();

    void* OnMetamodQuery(const char* iface, int* ret) override;

public:
    bool SteamBanAllowAccount(uint64_t steamId64) override;
    bool SteamBanDisallowAccount(uint64_t steamId64) override;

    bool Load(PluginId id, ISmmAPI* ismm, char* error, size_t maxlen, bool late) override;
    bool Unload(char* error, size_t maxlen) override;

    const char* GetAuthor() override;
    const char* GetName() override;
    const char* GetDescription() override;
    const char* GetURL() override;
    const char* GetLicense() override;
    const char* GetVersion() override;
    const char* GetDate() override;
    const char* GetLogTag() override;

public:
    KHook::Return<void> CSource2Server_GameFrame(ISource2Server* pThis, bool simulating, bool bFirstTick, bool bLastTick);
    KHook::Return<void> INetworkServerService_StartupServer(INetworkServerService* pThis, const GameSessionConfiguration_t& config, ISource2WorldSession* pSession, const char* pszMapName);
    KHook::Return<int> CGameEventManager_LoadEventsFromFile(IGameEventManager2* pThis, const char* pszFilename, bool bSearchAll);

    KHook::Virtual<ISource2Server, void, bool, bool, bool>* m_hGameFrame = nullptr;
    KHook::Virtual<INetworkServerService, void, const GameSessionConfiguration_t&, ISource2WorldSession*, const char*>* m_hStartupServer = nullptr;
    KHook::Virtual<IGameEventManager2, int, const char*, bool>* m_hLoadEventsFromFile = nullptr;

private:
    std::vector<std::unique_ptr<CFix>> CreateFixes();
    CFix* FindFix(const char* pszName) const;
    bool LoadFixes(const FixModules& modules, char* error, size_t maxlen);

    std::vector<std::unique_ptr<CFix>> m_fixes;

    // No interface to fetch: the first LoadEventsFromFile hands the instance over.
    VirtualTable m_GameEventManagerVTable;
    IGameEventManager2* m_pGameEventManager = nullptr;
};

extern Plugin g_Plugin;

PLUGIN_GLOBALVARS();
