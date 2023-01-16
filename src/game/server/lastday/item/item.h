#ifndef GAME_SERVER_LASTDAY_ITEM_H
#define GAME_SERVER_LASTDAY_ITEM_H

#include "inventory.h"
#include "item-data.h"

class CItemCore
{
    friend class CMakeCore;

    class CGameContext *m_pGameServer;
	CGameContext *GameServer() const { return m_pGameServer; }

    class CMakeCore *m_pMake;

	array<CItemData> m_aItems;
    CInventory m_aInventories[MAX_CLIENTS];

    void InitItem();
public:
    CItemCore(CGameContext *pGameServer);
    class CMakeCore *Make() const {return m_pMake;}

    CItemData *GetItemData(const char* Name);
    CInventory *GetInventory(int ClientID);
    bool IsWeaponHaveAmmo(int Weapon);
    int GetInvItemNum(const char *ItemName, int ClientID);
    void AddInvItemNum(const char *ItemName, int Num, int ClientID);
    void SetInvItemNum(const char *ItemName, int Num, int ClientID);
    void ClearInv(int ClientID);
};

#endif