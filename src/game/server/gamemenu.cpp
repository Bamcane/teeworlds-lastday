#include "lastday/item/make.h"
#include "gamecontext.h"
#include "gamemenu.h"

CMenu::CMenu(CGameContext *pGameServer) :
    m_pGameServer(pGameServer)
{
    str_copy(m_aLanguageCode, "en", sizeof(m_aLanguageCode));
    for(int i = 0;i < MAX_CLIENTS;i++)
    {
        m_aMenuChat[i].clear();
    }
}

const char* CMenu::Localize(const char* pText) const
{
	return GameServer()->Localize(m_aLanguageCode, pText);
}

void CMenu::GetData(int Page)
{
    m_DataTemp.clear();

    for(int i = 0;i < m_apOptions.size();i ++)
    {
        if(m_apOptions[i]->m_Page == Page)
            m_DataTemp.add(m_apOptions[i]->m_pName);
        else if(Page != MENUPAGE_MAIN && m_apOptions[i]->m_Page == MENUPAGE_NOTMAIN)
            m_DataTemp.add(m_apOptions[i]->m_pName);
    }
}

int CMenu::FindOption(const char *pName, int Pages)
{
	for(int i = 0;i < m_apOptions.size();i ++)
	{
        if(str_comp_nocase(m_apOptions[i]->m_pName, pName) == 0)
        {
            if(m_apOptions[i]->m_Page == Pages)
                return i;
            else if(Pages != MENUPAGE_MAIN && m_apOptions[i]->m_Page == MENUPAGE_NOTMAIN)
                return i;
        }
	}

	return -1;
}

void CMenu::Register(const char *pName, int Pages, MenuCallback pfnFunc, void *pUser, bool CloseMenu)
{
	int OptionID = FindOption(pName, Pages);
    if(OptionID > -1)
        return;

	COptions *pOption = new(mem_alloc(sizeof(COptions), sizeof(void*))) COptions;
	
	pOption->m_pfnCallback = pfnFunc;

    pOption->m_OptionType = MENUOPTION_OPTIONS;
	pOption->m_pName = pName;
	pOption->m_Page = Pages;
    pOption->m_pUserData = pUser;
    pOption->m_CloseMenu = CloseMenu;

    m_apOptions.add(pOption);
}

void CMenu::RegisterMake(const char *pName)
{
	COptions *pOption = new(mem_alloc(sizeof(COptions), sizeof(void*))) COptions;

    pOption->m_OptionType = MENUOPTION_ITEMS;
	pOption->m_pfnCallback = 0;
    pOption->m_pUserData = GameServer();
	pOption->m_pName = pName;
	pOption->m_Page = MENUPAGE_ITEM;
    pOption->m_CloseMenu = 0;

    m_apOptions.add(pOption);
}

void CMenu::RegisterLanguage(const char *pName)
{
	COptions *pOption = new(mem_alloc(sizeof(COptions), sizeof(void*))) COptions;

    pOption->m_OptionType = MENUOPTION_LANGUAGE;
	pOption->m_pfnCallback = 0;
    pOption->m_pUserData = GameServer();
	pOption->m_pName = pName;
	pOption->m_Page = MENUPAGE_LANGUAGE;
    pOption->m_CloseMenu = 0;

    m_apOptions.add(pOption);
}

void CMenu::ShowMenu(int ClientID, int Line)
{
    CPlayer *pPlayer = GameServer()->m_apPlayers[ClientID];

    if(!pPlayer)
        return;
    
    str_copy(m_aLanguageCode, pPlayer->GetLanguage(), sizeof(m_aLanguageCode));

    std::string MenuBuffer;
    int Page;

    Page = pPlayer->GetMenuPage();

    GetData(Page);
    if(!m_DataTemp.size())
        return;
        
    MenuBuffer.append("===");
    MenuBuffer.append(Localize("Menu"));
    MenuBuffer.append("===");
    MenuBuffer.append("\n");
    if (Line < 0)
    {
        Line = m_DataTemp.size() + Line%m_DataTemp.size();
    }

    if (Line >= m_DataTemp.size())
    {
        Line = 0 + Line%m_DataTemp.size();
    }


    if(m_DataTemp.size() > 7)
    {
        int j = Line;
        for(int i = 0;i < 7; i++)
        {
            if(j < 0)
                j = m_DataTemp.size()-1;

            if(j >= m_DataTemp.size())
                j = 0;

            const char* Buffer = m_DataTemp[j].c_str();
            if(i == 3)
                MenuBuffer.append("[");
            MenuBuffer.append(Localize(Buffer));
            if(i == 3)
                MenuBuffer.append("]");
            MenuBuffer.append("\n");

            if(i == 3)
            {
                pPlayer->m_SelectOption = Buffer;
            }

            j++;
        }
    }
    else
    {

        int j = 0;
        for(int i = 0;i < m_DataTemp.size(); i++)
        {
            const char* Buffer = m_DataTemp[j].c_str();
            if(j == Line)
                MenuBuffer.append("[");
            else MenuBuffer.append(" ");
            MenuBuffer.append(Localize(Buffer));
            if(j == Line)
                MenuBuffer.append("]");
            MenuBuffer.append("\n");

            if(j == Line)
            {
                pPlayer->m_SelectOption = Buffer;
            }

            j++;
        }
    }
    
    if(Page == MENUPAGE_MAIN)
    {
        MenuBuffer.append(Localize("\n"));
        MenuBuffer.append(Localize("Use <mouse1>(Fire) to use option"));
    }

    if(Page == MENUPAGE_ITEM)
    {
        MenuBuffer.append(Localize("\n"));
        MenuBuffer.append(Localize("Use <mouse2>(Hook) to back main menu"));
        CItemData *ItemInfo = GameServer()->Item()->GetItemData(pPlayer->m_SelectOption);

        std::string Buffer;
        Buffer.clear();
        
        for(int i = 0; i < ItemInfo->m_Needs.m_Name.size();i ++)
        {
            Buffer.append("\n");
            Buffer.append(Localize(ItemInfo->m_Needs.m_Name[i].c_str()));
            Buffer.append(":");
            Buffer.append(std::to_string(GameServer()->Item()->GetInvItemNum(ItemInfo->m_Needs.m_Name[i].c_str(), ClientID)));
            Buffer.append("/");
            Buffer.append(std::to_string(ItemInfo->m_Needs.m_Num[i]));   
        }

        MenuBuffer.append("\n\n");
        std::string TempBuffer;
        GameServer()->Server()->Localization()->Format_L(TempBuffer, m_aLanguageCode, "%s requires", pPlayer->m_SelectOption);
        MenuBuffer.append(TempBuffer);
        MenuBuffer.append(":");
        MenuBuffer.append(Buffer);
    }

    if(m_aMenuChat[ClientID].length())
    {
        MenuBuffer.append("\n\n");
        MenuBuffer.append(Localize("Chat"));
        MenuBuffer.append(":");
        MenuBuffer.append(m_aMenuChat[ClientID]);
        m_aMenuChat[ClientID].clear();
    }

    GameServer()->SendMotd(ClientID, MenuBuffer.c_str());

}

void CMenu::UseOptions(int ClientID)
{
    CPlayer *pPlayer = GameServer()->m_apPlayers[ClientID];

    if(!pPlayer)
        return;
    
    int OptionID = FindOption(pPlayer->m_SelectOption, pPlayer->GetMenuPage());

    if(OptionID == -1 || OptionID >= m_apOptions.size())
    {
        dbg_msg("menu", "A player use invalid option %s Page: %d", pPlayer->m_SelectOption, pPlayer->GetMenuPage());
        return;
    }
    
    if(m_apOptions[OptionID]->m_CloseMenu)
        pPlayer->CloseMenu();
    else
    {
        ShowMenu(ClientID, pPlayer->m_MenuLine);
        pPlayer->m_MenuCloseTick = MENU_CLOSETICK;
    }

    if(m_apOptions[OptionID]->GetOptionType() == MENUOPTION_OPTIONS)
        m_apOptions[OptionID]->m_pfnCallback(ClientID, m_apOptions[OptionID]->m_pUserData);
    else if(m_apOptions[OptionID]->GetOptionType() == MENUOPTION_ITEMS)
        GameServer()->MakeItem(ClientID, m_apOptions[OptionID]->m_pName);
    else if(m_apOptions[OptionID]->GetOptionType() == MENUOPTION_LANGUAGE)
        GameServer()->SetPlayerLanguage(ClientID, m_apOptions[OptionID]->m_pName);
}

void CMenu::AddMenuChat(int ClientID, const char *pChat)
{
    m_aMenuChat[ClientID] = pChat;

    CPlayer *pPlayer = GameServer()->m_apPlayers[ClientID];

    if(pPlayer)
    {
        ShowMenu(ClientID, pPlayer->m_MenuLine);
        pPlayer->m_MenuCloseTick = MENU_CLOSETICK;
    }
}