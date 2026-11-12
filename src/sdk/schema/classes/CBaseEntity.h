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

#ifndef _INCLUDE_CBASEENTITY_H
#define _INCLUDE_CBASEENTITY_H
#ifdef _WIN32
#pragma once
#endif

#include "CBodyComponent.h"
#include "CCollisionProperty.h"
#include "sdk/schemasystem_helper.h"

#include <const.h>
#include <entityhandle.h>
#include <entityinstance.h>
#include <mathlib/vector.h>

class CBaseEntity : public CEntityInstance
{
public:
    SCHEMA_FIELD(CBodyComponent*, CBaseEntity, m_CBodyComponent);
    SCHEMA_FIELD(uint32_t, CBaseEntity, m_fFlags);
    SCHEMA_FIELD(MoveType_t, CBaseEntity, m_MoveType);
    SCHEMA_FIELD(MoveType_t, CBaseEntity, m_nActualMoveType);
    SCHEMA_FIELD(Vector, CBaseEntity, m_vecAbsVelocity);
    SCHEMA_FIELD(CCollisionProperty*, CBaseEntity, m_pCollision);
    SCHEMA_FIELD(CEntityHandle, CBaseEntity, m_hOwnerEntity);
    SCHEMA_FIELD(CEntityHandle, CBaseEntity, m_hGroundEntity);
};

#endif
