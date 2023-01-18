
#include <engine/external/nlohmann/json.hpp>
#include <game/server/gamecontext.h>

#include <fstream>

#include "item.h"
#include "make.h"

using json = nlohmann::json;

CItemCore::CItemCore(CGameContext *pGameServer)
{
    m_pGameServer = pGameServer;
    m_pMake = new CMakeCore(this);

	InitItem();
}

void CItemCore::InitItem()
{
    // read file data into buffer
	const char *pFilename = "./data/json/item.json";
	std::ifstream File(pFilename);

	if(!File.is_open())
	{
		dbg_msg("Item", "can't open 'data/json/item.json'");
		return;
	}

	// parse json data
	json Data = json::parse(File);

	json ItemArray = Data["json"];
	if(ItemArray.is_array())
	{
		for(unsigned i = 0; i < ItemArray.size(); ++i)
		{
			CItemData *pData = new CItemData();
			str_copy(pData->m_aName, ItemArray[i].value("name", " ").c_str());
			pData->m_IsMakeable = ItemArray[i].value("makeable", 1);
			pData->m_WeaponAmmoID = ItemArray[i].value("weapon_ammo", -1);
			pData->m_WeaponID = ItemArray[i].value("weapon", -1);
			pData->m_GiveNum = ItemArray[i].value("give_num", 1);
			pData->m_Health = ItemArray[i].value("health", 0);
			
			if(pData->m_IsMakeable)
			{
				json NeedArray = ItemArray[i]["need"];

				if(NeedArray.is_array())
				{
					CMakeData *pNeed = new CMakeData();
					for(unsigned j = 0;j < NeedArray.size();j++)
					{
						pNeed->m_Name.add(NeedArray[j].value("name", " ").c_str());
						pNeed->m_Num.add(NeedArray[j].value("num", 0));
					}
					pData->m_Needs = *pNeed;
					GameServer()->Menu()->RegisterMake(pData->m_aName);
				}
			}

			m_aItems.add(*pData);
		}
	}
}

CItemData *CItemCore::GetItemData(const char *Name)
{
    for(unsigned i = 0;i < m_aItems.size();i ++)
    {
        if(str_comp(m_aItems[i].m_aName, Name) == 0)
            return &m_aItems[i];
    }
    return 0x0;
}

CInventory *CItemCore::GetInventory(int ClientID)
{
	return &m_aInventories[ClientID];
}

bool CItemCore::IsWeaponHaveAmmo(int Weapon)
{
	for(int i = 0;i < m_aItems.size();i++)
	{
		if(m_aItems[i].m_WeaponAmmoID == i)
		{
			return 1;
		}
	}
	return 0;
}

int CItemCore::GetInvItemNum(const char *ItemName, int ClientID)
{
	for(int i = 0;i < m_aInventories[ClientID].m_Datas.size();i ++)
	{
		if(str_comp(m_aInventories[ClientID].m_Datas[i].m_aName, ItemName) == 0)
		{
			return m_aInventories[ClientID].m_Datas[i].m_Num;
		}
	}
	return 0;
}

void CItemCore::AddInvItemNum(const char *ItemName, int Num, int ClientID, bool Database)
{
	bool Added = false;
	int DatabaseNum = Num;
	for(int i = 0;i < m_aInventories[ClientID].m_Datas.size();i ++)
	{
		if(str_comp(m_aInventories[ClientID].m_Datas[i].m_aName, ItemName) == 0)
		{
			m_aInventories[ClientID].m_Datas[i].m_Num += Num;
			DatabaseNum = m_aInventories[ClientID].m_Datas[i].m_Num;
			Added = true;
			break;
		}
	}

	if(!Added)
	{
		CInventoryData Data;
		str_copy(Data.m_aName, ItemName);
		Data.m_Num = Num;

		m_aInventories[ClientID].m_Datas.add(Data);
	}
	GameServer()->Postgresql()->CreateUpdateItemThread(ClientID, ItemName, DatabaseNum);
}

void CItemCore::SetInvItemNum(const char *ItemName, int Num, int ClientID, bool Database)
{
	bool Set = false;
	for(int i = 0;i < m_aInventories[ClientID].m_Datas.size();i ++)
	{
		if(str_comp(m_aInventories[ClientID].m_Datas[i].m_aName, ItemName) == 0)
		{
			m_aInventories[ClientID].m_Datas[i].m_Num = Num;
			Set = true;
			break;
		}
	}

	if(!Set)
	{
		CInventoryData Data;
		str_copy(Data.m_aName, ItemName);
		Data.m_Num = Num;

		m_aInventories[ClientID].m_Datas.add(Data);
	}
	if(Database)
	{
		GameServer()->Postgresql()->CreateUpdateItemThread(ClientID, ItemName, Num);
	}
}

void CItemCore::ClearInv(int ClientID, bool Database)
{
	m_aInventories[ClientID].m_Datas.clear();
	if(Database)
	{
		GameServer()->Postgresql()->CreateClearItemThread(ClientID);
	}
}