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
#include "sdk/game/shared/movedata.h"
#include "utils.hpp"

#include <gametrace.h>
#include <mathlib/vector.h>

class CBasePlayerPawn;
class CCSPlayer_MovementServices;
struct PlayerBounds_t
{
    Vector mins;
    Vector maxs;
};

class CRampbugFix final : public CFix
{
public:
    CRampbugFix();

    const char* GetName() const override { return "rampbug"; }

    void ReadConfig(KeyValues* pConfig) override;
    bool Load(const FixModules& modules, char* error, size_t maxlen) override;
    void Unload() override;

    void OnStartupServer(const GameSessionConfiguration_t& config, const char* pszMapName) override;

public:
    KHook::Return<void> CCSPlayer_MovementServices_ProcessMovement(CCSPlayer_MovementServices* pThis, void* pMove);
    KHook::Return<void> CCSPlayer_MovementServices_ProcessMovementPost(CCSPlayer_MovementServices* pThis, void* pMove);
    KHook::Return<void> CCSPlayer_MovementServices_TryPlayerMove(CCSPlayer_MovementServices* pThis, CMoveData* pMove, Vector* pFirstDest, trace_t* pFirstTrace, bool* pIsSurfing);
    KHook::Return<void> CCSPlayer_MovementServices_TryPlayerMovePost(CCSPlayer_MovementServices* pThis, CMoveData* pMove, Vector* pFirstDest, trace_t* pFirstTrace, bool* pIsSurfing);
    KHook::Return<void> CCSPlayer_MovementServices_CategorizePosition(CCSPlayer_MovementServices* pThis, CMoveData* pMove, bool bStayOnGround);

    KHook::Member<CCSPlayer_MovementServices, void, void*>* m_hProcessMovement = nullptr;
    KHook::Member<CCSPlayer_MovementServices, void, CMoveData*, Vector*, trace_t*, bool*>* m_hTryPlayerMove = nullptr;
    KHook::Member<CCSPlayer_MovementServices, void, CMoveData*, bool>* m_hCategorizePosition = nullptr;

private:
    struct PlayerState
    {
        CMoveData* pMoveData = nullptr;       // the pass in flight, null outside ProcessMovement
        bool bProcessingMovement = false;
        bool bPreviousOnGround = false;
        int nGameCodeMoves = 0;
        bool bDidTPM = false;
        bool bOverrideTPM = false;
        Vector vecTPMVelocity;                // vec3_invalid until TryPlayerMove ran
        Vector vecTPMOrigin;
        Vector vecLastValidPlane;             // vec3_origin when there is none
        Vector vecTakeoffVelocity;
        Vector vecLandingOrigin;
        Vector vecLandingVelocity;

        void Reset();
        void ForgetSurface();
    };

    bool m_bOverrideTryPlayerMove = false;

    PlayerState* GetState(CCSPlayer_MovementServices* pServices, CBasePlayerPawn** ppPawn);

    static void GetOrigin(const PlayerState& state, CBasePlayerPawn* pPawn, Vector& vecOut);
    static void GetVelocity(const PlayerState& state, CBasePlayerPawn* pPawn, Vector& vecOut);
    static void SetOrigin(PlayerState& state, CBasePlayerPawn* pPawn, const Vector& vecOrigin);
    static void SetVelocity(PlayerState& state, CBasePlayerPawn* pPawn, const Vector& vecVelocity);
    static void GetBounds(CCSPlayer_MovementServices* pServices, PlayerBounds_t& bounds);

    void RegisterLanding(PlayerState& state, CBasePlayerPawn* pPawn, const Vector& vecVelocity);
    void ApplySlopeFix(PlayerState& state, CBasePlayerPawn* pPawn);
    bool IsValidMovementTrace(trace_t& trace, const PlayerBounds_t& bounds, CTraceFilter* pFilter);

    void TracePlayerBBox(const Vector& start, const Vector& end, const PlayerBounds_t& bounds, CTraceFilter* pFilter, trace_t& pm);
    void* m_pNavPhysicsVTable = nullptr;

    PlayerState m_states[64];
};
