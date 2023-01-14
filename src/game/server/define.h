#ifndef GAME_SERVER_DEFINE_H
#define GAME_SERVER_DEFINE_H

#include <base/system.h>

#define MENU_CLOSETICK 200

char* format_int64_with_commas(char commas, int64 n);

enum OptionType
{
    MENUOPTION_OPTIONS=0,
    MENUOPTION_ITEMS,

    NUM_MENUOPTIONS,
};

enum LastDayWeapons
{
    LD_WEAPON_HAMMER=0,
    LD_WEAPON_GUN,
    LD_WEAPON_SHOTGUN,
    LD_WEAPON_GRENADE,
    LD_WEAPON_RIFLE,
    LD_WEAPON_NINJA,

    LD_WEAPON_FREEZE_RIFLE,

    NUM_LASTDAY_WEAPONS,
};

enum MenuPages
{
    MENUPAGE_MAIN=0,
    MENUPAGE_NOTMAIN,
    MENUPAGE_ITEM,
};

class CBotData
{
public:
    CBotData() {};
    char m_SkinName[64];
    int m_BodyColor;
    int m_FeetColor;
    int m_AttackProba;
    int m_SpawnProba;
    int m_DropProba;
    int m_DropNum;
    bool m_TeamDamage;
    bool m_Hammer;
    bool m_Gun;
    bool m_Hook;
};

#endif