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

//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======

#ifndef MOVEDATA_H
#define MOVEDATA_H
#ifdef _WIN32
#pragma once
#endif

#include <entityhandle.h>
#include <gametrace.h>
#include <mathlib/vector.h>
#include <tier0/utlvector.h>

struct SubtickMove
{
    float when;
    uint64 button;
    bool pressed;
};

struct MovementVector2D
{
    float x;
    float y;
};

struct touchlist_t
{
    Vector deltavelocity;
    trace_t trace;
};

class CMoveDataBase
{
public:
    bool m_bHasZeroFrametime : 1;
    bool m_bIsLateCommand : 1;
    CEntityHandle m_nPlayerHandle;
    QAngle m_vecAbsViewAngles;
    QAngle m_vecViewAngles;
    Vector m_vecLastMovementImpulses;
    float m_flForwardMove;
    float m_flSideMove;  // flipped against CS:GO: moving right is negative
    float m_flUpMove;
    Vector m_vecVelocity;
    QAngle m_vecAngles;
    Vector m_vecUnknown;
    CUtlVector<SubtickMove> m_SubtickMoves;
    CUtlVector<SubtickMove> m_AttackSubtickMoves;
    bool m_bHasSubtickInputs;
    float unknown;
    CUtlVector<touchlist_t> m_TouchList;
    Vector m_collisionNormal;
    Vector m_groundNormal;
    Vector m_vecAbsOrigin;
    int32 m_nTickCount;
    int32 m_nTargetTick;
    float m_flSubtickStartFraction;
    float m_flSubtickEndFraction;
};

class CMoveData : public CMoveDataBase
{
public:
    Vector m_outWishVel;
    QAngle m_vecOldAngles;
    MovementVector2D m_vecWalkWishVel;
    Vector m_vecContinousAcceleration;  // u/s^2
    Vector m_vecFrameVelocityDelta;     // u/s
    float m_flMaxSpeed;
};

#endif
