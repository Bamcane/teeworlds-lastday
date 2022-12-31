
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
			pData->m_IsDrops = ItemArray[i].value("drops", 0);
			pData->m_WeaponAmmoID = ItemArray[i].value("weapon_ammo", -1);
			pData->m_WeaponID = ItemArray[i].value("weapon", -1);
			pData->m_GiveNum = ItemArray[i].value("give_num", 1);
			
			if(pData->m_IsMakeable)
			{
				json NeedArray = ItemArray[i]["need"];

				if(NeedArray.is_array())
				{
					CInventoryData *pNeed = new CInventoryData();
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

			if(pData->m_IsDrops)
			{
				m_aDrops.add(pData);
			}
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

CInventoryData *CItemCore::GetInventory(int ClientID)
{
	return &m_aInventories[ClientID];
}

int CItemCore::GetInvItemNum(const char *ItemName, int ClientID)
{
	for(int i = 0;i < m_aInventories[ClientID].m_Name.size();i ++)
	{
		if(str_comp(m_aInventories[ClientID].m_Name[i].c_str(), ItemName) == 0)
		{
			return m_aInventories[ClientID].m_Num[i];
		}
	}
	return 0;
}

void CItemCore::AddInvItemNum(const char *ItemName, int Num, int ClientID)
{
	bool Added = false;
	for(int i = 0;i < m_aInventories[ClientID].m_Name.size();i ++)
	{
		if(str_comp(m_aInventories[ClientID].m_Name[i].c_str(), ItemName) == 0)
		{
			m_aInventories[ClientID].m_Num[i] += Num;
			Added = true;
			break;
		}
	}

	if(!Added)
	{
		m_aInventories[ClientID].m_Name.add(ItemName);
		m_aInventories[ClientID].m_Num.add(Num);
	}
}

void CItemCore::SetInvItemNum(const char *ItemName, int Num, int ClientID)
{
	bool Set = false;
	for(int i = 0;i < m_aInventories[ClientID].m_Name.size();i ++)
	{
		if(str_comp(m_aInventories[ClientID].m_Name[i].c_str(), ItemName) == 0)
		{
			m_aInventories[ClientID].m_Num[i] = Num;
			Set = true;
			break;
		}
	}

	if(!Set)
	{
		m_aInventories[ClientID].m_Name.add(ItemName);
		m_aInventories[ClientID].m_Num.add(Num);
	}
}