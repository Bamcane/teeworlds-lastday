#ifndef GAME_SERVER_LASTDAY_WEAPONS_FREEZE_RIFLE_H
#define GAME_SERVER_LASTDAY_WEAPONS_FREEZE_RIFLE_H

#include <game/server/lastday/weapons-core/weapon.h>

class CWeaponFreezeRifle : public CWeapon
{
public:
    CWeaponFreezeRifle(CGameContext *pGameServer);

    void Fire(int Owner, vec2 Dir, vec2 Pos) override;
};

#endif