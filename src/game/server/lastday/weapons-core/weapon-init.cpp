#include "weapon.h"


#include <game/server/lastday/weapons/hammer.h>
#include <game/server/lastday/weapons/gun.h>
#include <game/server/lastday/weapons/shotgun.h>
#include <game/server/lastday/weapons/grenade.h>
#include <game/server/lastday/weapons/rifle.h>
#include <game/server/lastday/weapons/ninja.h>
#include <game/server/lastday/weapons/freeze-rifle.h>

void WeaponInit::InitWeapons(CGameContext *pGameServer)
{
	g_Weapons.InitWeapon(LD_WEAPON_HAMMER, new CWeaponHammer(pGameServer));
	g_Weapons.InitWeapon(LD_WEAPON_GUN, new CWeaponGun(pGameServer));
	g_Weapons.InitWeapon(LD_WEAPON_SHOTGUN, new CWeaponShotgun(pGameServer));
	g_Weapons.InitWeapon(LD_WEAPON_GRENADE, new CWeaponGrenade(pGameServer));
	g_Weapons.InitWeapon(LD_WEAPON_RIFLE, new CWeaponRifle(pGameServer));
	g_Weapons.InitWeapon(LD_WEAPON_NINJA, new CWeaponNinja(pGameServer));
	g_Weapons.InitWeapon(LD_WEAPON_FREEZE_RIFLE, new CWeaponFreezeRifle(pGameServer));
}