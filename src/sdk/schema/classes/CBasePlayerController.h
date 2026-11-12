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

#ifndef _INCLUDE_CBASEPLAYERCONTROLLER_H
#define _INCLUDE_CBASEPLAYERCONTROLLER_H
#ifdef _WIN32
#pragma once
#endif

#include "CBaseEntity.h"

#include <const.h>
#include <entityhandle.h>
#include <entitysystem.h>
#include <gameentitysystem.h>

enum class PlayerConnectedState : uint32_t
{
    PlayerNeverConnected = 0xFFFFFFFF,
    PlayerConnected = 0x0,
    PlayerConnecting = 0x1,
    PlayerReconnecting = 0x2,
    PlayerDisconnecting = 0x3,
    PlayerDisconnected = 0x4,
    PlayerReserved = 0x5,
};

class CBasePlayerController : public CBaseEntity
{
public:
    SCHEMA_FIELD(PlayerConnectedState, CBasePlayerController, m_iConnected);
    SCHEMA_FIELD(bool, CBasePlayerController, m_bIsHLTV);
    SCHEMA_FIELD(CEntityHandle, CBasePlayerController, m_hPawn);
    SCHEMA_FIELD_POINTER(char, CBasePlayerController, m_iszPlayerName);

    bool IsBot() { return (m_fFlags() & FL_FAKECLIENT) != 0; }
    bool IsConnected() { return m_iConnected() == PlayerConnectedState::PlayerConnected; }
    bool IsHLTV() { return m_bIsHLTV(); }

    CBaseEntity* GetPawn() { return static_cast<CBaseEntity*>(GameEntitySystem()->GetEntityInstance(m_hPawn())); }
    const char* GetPlayerName() { return m_iszPlayerName(); }
};

#endif
