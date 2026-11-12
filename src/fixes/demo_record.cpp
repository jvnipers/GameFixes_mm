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

#include "demo_record.h"

#include <engine2/gamesessionconfiguration.h>
#include "utils.hpp"
#include "vprof.hpp"

#include <cstdio>

CDemoRecordFix::CDemoRecordFix() :
    KHOOK_NEW(m_hDisconnect, &CServerSideClientBase::Disconnect, this, &CDemoRecordFix::CServerSideClient_Disconnect, nullptr)
{
}

bool CDemoRecordFix::Load(const FixModules& modules, char* error, size_t maxlen)
{
    m_tvEnable = std::make_unique<CConVarRef<bool>>("tv_enable");

    void* pVTable = sig::FindVirtualTable(modules.engine, "CServerSideClient");
    if (!pVTable)
    {
        std::snprintf(error, maxlen, "CServerSideClient vtable not found");
        return false;
    }

    m_VTable.m_pVTFs = static_cast<void**>(pVTable);
    m_hDisconnect->AddGlobal(AsHookTarget<CServerSideClientBase>(m_VTable));

    Log("hooked CServerSideClient::Disconnect on vtable %p, tv_enable is %s", pVTable, IsTvEnabled() ? "on" : "off");
    return true;
}

void CDemoRecordFix::Unload()
{
    if (m_VTable.m_pVTFs && m_hDisconnect)
        m_hDisconnect->RemoveGlobal(AsHookTarget<CServerSideClientBase>(m_VTable));

    delete m_hDisconnect;
    m_hDisconnect = nullptr;
    m_VTable.m_pVTFs = nullptr;
    m_tvEnable.reset();
}

bool CDemoRecordFix::IsTvEnabled() const
{
    return m_tvEnable && m_tvEnable->IsValidRef() && m_tvEnable->Get();
}

void CDemoRecordFix::OnStartupServer(const GameSessionConfiguration_t& config, const char* pszMapName)
{
    if (!IsTvEnabled())
        return;

    // The engine reads max_clients after this Pre hook, so GOTV gets a slot of its own.
    Log("GOTV is enabled, expanding the session's maxplayers (%u -> %u)", config.max_clients(), config.max_clients() + 1);
    const_cast<GameSessionConfiguration_t&>(config).set_max_clients(config.max_clients() + 1);
}

KHook::Return<void> CDemoRecordFix::CServerSideClient_Disconnect(CServerSideClientBase* pThis, ENetworkDisconnectionReason reason, const char* pszInternalReason)
{
    GF_VPROF("GameFixes::demo_record::Disconnect");

    if (pThis->m_bIsHLTV && IsTvEnabled())
    {
        Log("GOTV is enabled, blocking the GOTV client's disconnect");
        return { KHook::Action::Supersede };
    }

    return { KHook::Action::Ignore };
}
