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

#include <cstddef>

class KeyValues;
class GameSessionConfiguration_t;
class IGameEventManager2;

// Any pointer inside each module.
struct FixModules
{
    const void* server;
    const void* engine;
};

class CFix
{
public:
    virtual ~CFix() = default;

    virtual const char* GetName() const = 0;

    virtual void ReadConfig(KeyValues* pConfig) {}

    // KHook hooks come down only in their destructor: own them by pointer and delete them in Unload(), which also runs for a fix that never loaded.
    virtual bool Load(const FixModules& modules, char* error, size_t maxlen) = 0;
    virtual void Unload() {}

    // StartupServer is Pre, so config is still editable.
    virtual void OnStartupServer(const GameSessionConfiguration_t& config, const char* pszMapName) {}
    virtual void OnGameFrame(bool simulating) {}

    virtual void OnGameEventManagerReady(IGameEventManager2* pManager) {}

protected:
    void Log(const char* pszFormat, ...) const;
};
