#ifndef GAME_SERVER_LASTDAY_ITEM_DATA_H
#define GAME_SERVER_LASTDAY_ITEM_DATA_H

#include <base/tl/array.h>
#include <string>

class CMakeData
{
public:
    CMakeData();
    array<std::string> m_Name;
    array<int> m_Num;
};

class CItemData
{
public:
	CItemData();
	char m_aName[128];
    bool m_IsMakeable;
    bool m_IsDrops;
	int m_WeaponID;
	int m_WeaponAmmoID;
    int m_GiveNum;
    CMakeData m_Needs;
};

#endif