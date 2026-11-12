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

#ifndef _INCLUDE_CCSPLAYER_MOVEMENTSERVICES_H
#define _INCLUDE_CCSPLAYER_MOVEMENTSERVICES_H
#ifdef _WIN32
#pragma once
#endif

#include "CPlayer_MovementServices_Humanoid.h"
#include "sdk/schemasystem_helper.h"

class CCSPlayer_MovementServices : public CPlayer_MovementServices_Humanoid
{
public:
    SCHEMA_FIELD(bool, CCSPlayer_MovementServices, m_bDucked);
    // Counted up whenever game code moves the player outside the movement pass (a teleport)
    SCHEMA_FIELD(int32_t, CCSPlayer_MovementServices, m_nGameCodeHasMovedPlayerAfterCommand);
};

#endif
