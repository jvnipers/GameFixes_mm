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

class CCSPlayer_MovementServices;
class CMoveData;

class CWaterJumpFix final : public CFix
{
public:
    CWaterJumpFix();

    const char* GetName() const override { return "water_jump"; }

    bool Load(const FixModules& modules, char* error, size_t maxlen) override;
    void Unload() override;

    void OnStartupServer(const GameSessionConfiguration_t& config, const char* pszMapName) override;

public:
    KHook::Return<void> CCSPlayer_MovementServices_WaterMove(CCSPlayer_MovementServices* pThis, CMoveData* pMove);
    KHook::Return<void> CCSPlayer_MovementServices_CategorizePosition(CCSPlayer_MovementServices* pThis, CMoveData* pMove, bool bStayOnGround);

    KHook::Member<CCSPlayer_MovementServices, void, CMoveData*>* m_hWaterMove = nullptr;
    KHook::Member<CCSPlayer_MovementServices, void, CMoveData*, bool>* m_hCategorizePosition = nullptr;

private:
    static int GetSlot(CCSPlayer_MovementServices* pServices);

    bool m_bSkipNextCategorize[ABSOLUTE_PLAYER_LIMIT] = {};
};
