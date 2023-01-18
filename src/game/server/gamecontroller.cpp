/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/shared/config.h>
#include <game/mapitems.h>
#include <game/version.h>

#include <game/generated/protocol.h>

#include "entities/pickup.h"

#include "gamecontroller.h"
#include "gamecontext.h"

#include <engine/external/nlohmann/json.hpp>

#include <fstream>
#include <string.h>

using json = nlohmann::json;

static LOCK BotDataLock = 0;

CGameController::CGameController(class CGameContext *pGameServer)
{
	m_pGameServer = pGameServer;
	m_pServer = m_pGameServer->Server();
	m_pGameType = MOD_NAME;

	//
	m_UnpauseTimer = 0;
	m_GameOverTick = -1;
	m_SuddenDeath = 0;
	m_RoundStartTick = Server()->Tick();
	m_RoundCount = 0;
	m_GameFlags = 0;
	m_aMapWish[0] = 0;

	m_UnbalancedTick = -1;
	m_ForceBalanced = false;
	
	m_SpawnPoints.clear();

	BotDataLock = lock_create();

	m_BotDataInit = false;
	
	WeaponIniter.InitWeapons(pGameServer);

	InitBotData();
}

CGameController::~CGameController()
{
}

vec2 CGameController::GetSpawnPos()
{
	vec2 Pos;
	for(int i = 0;i < m_SpawnPoints.size(); i++)
	{
		Pos = m_SpawnPoints[random_int(0, m_SpawnPoints.size()-1)];
		if(!GameServer()->m_World.ClosestCharacter(Pos, 128.0f, 0x0))
			break;
	}
	return Pos;
}

void CGameController::InitSpawnPos()
{
	// create all entities from the game layer
	CMapItemLayerTilemap *pTileMap = GameServer()->Layers()->GameLayer();
	CTile *pTiles = GameServer()->m_pTiles;

	for(int y = 0; y < pTileMap->m_Height; y++)
	{
		for(int x = 0; x < pTileMap->m_Width; x++)
		{
			int Index = pTiles[y*pTileMap->m_Width+x].m_Index;

			if(Index&CCollision::COLFLAG_SOLID || Index&CCollision::COLFLAG_DEATH) continue;
			int GroudIndex = pTiles[(y+1)*pTileMap->m_Width+x].m_Index;
			if(GroudIndex&CCollision::COLFLAG_SOLID && random_int(1, 100) >= y*100/pTileMap->m_Height)
			{
				vec2 Pos(x*32.0f+16.0f, y*32.0f+16.0f);
				m_SpawnPoints.add(Pos);
			}
		}
	}
}

bool CGameController::OnEntity(int Index, vec2 Pos)
{
	int Type = -1;
	int SubType = 0;
	return false;
}

void CGameController::EndRound()
{
	GameServer()->m_World.m_Paused = true;
	m_GameOverTick = Server()->Tick();
	m_SuddenDeath = 0;
}

void CGameController::ResetGame()
{
	GameServer()->m_World.m_ResetRequested = true;
}

const char *CGameController::GetTeamName(int Team)
{
	if(IsTeamplay())
	{
		if(Team == TEAM_RED)
			return "red team";
		else if(Team == TEAM_BLUE)
			return "blue team";
	}
	else
	{
		if(Team == 0)
			return "game";
	}

	return "spectators";
}

static bool IsSeparator(char c) { return c == ';' || c == ' ' || c == ',' || c == '\t'; }

void CGameController::StartRound()
{
	ResetGame();

	m_RoundStartTick = Server()->Tick();
	m_SuddenDeath = 0;
	m_GameOverTick = -1;
	GameServer()->m_World.m_Paused = false;
	m_ForceBalanced = false;
	Server()->DemoRecorder_HandleAutoStart();
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "start round type='%s' teamplay='%d'", m_pGameType, m_GameFlags&GAMEFLAG_TEAMS);
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
}

void CGameController::ChangeMap(const char *pToMap)
{
	str_copy(m_aMapWish, pToMap, sizeof(m_aMapWish));
	EndRound();
}

void CGameController::CycleMap()
{
	return;
}

void CGameController::PostReset()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(GameServer()->m_apPlayers[i])
		{
			GameServer()->m_apPlayers[i]->Respawn();
			GameServer()->m_apPlayers[i]->m_Score = 0;
			GameServer()->m_apPlayers[i]->m_ScoreStartTick = Server()->Tick();
			GameServer()->m_apPlayers[i]->m_RespawnTick = Server()->Tick()+Server()->TickSpeed()/2;
		}
	}
}

void CGameController::OnPlayerInfoChange(class CPlayer *pP)
{
	const int aTeamColors[2] = {65387, 10223467};
	if(IsTeamplay())
	{
		pP->m_TeeInfos.m_UseCustomColor = 1;
		if(pP->GetTeam() >= TEAM_RED && pP->GetTeam() <= TEAM_BLUE)
		{
			pP->m_TeeInfos.m_ColorBody = aTeamColors[pP->GetTeam()];
			pP->m_TeeInfos.m_ColorFeet = aTeamColors[pP->GetTeam()];
		}
		else
		{
			pP->m_TeeInfos.m_ColorBody = 12895054;
			pP->m_TeeInfos.m_ColorFeet = 12895054;
		}
	}
}


int CGameController::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon)
{
	// do scoreing
	if(!pKiller || Weapon == WEAPON_GAME)
		return 0;
	if(pKiller == pVictim->GetPlayer())
		pVictim->GetPlayer()->m_Score--; // suicide
	else
	{
		if(IsTeamplay() && pVictim->GetPlayer()->GetTeam() == pKiller->GetTeam())
			pKiller->m_Score--; // teamkill
		else
			pKiller->m_Score++; // normal kill
	}
	if(Weapon == WEAPON_SELF)
		pVictim->GetPlayer()->m_RespawnTick = Server()->Tick()+Server()->TickSpeed()*3.0f;

	Server()->ExpireServerInfo();
	return 0;
}

void CGameController::OnCharacterSpawn(class CCharacter *pChr)
{
	// default health
	pChr->IncreaseHealth(-1);

	// give default weapons
	GameServer()->Item()->SetInvItemNum("hammer", 1, pChr->GetCID(), 0);
}

void CGameController::TogglePause()
{
	if(IsGameOver())
		return;

	if(GameServer()->m_World.m_Paused)
	{
		GameServer()->m_World.m_Paused = false;
		m_UnpauseTimer = 0;
	}
	else
	{
		// pause
		GameServer()->m_World.m_Paused = true;
		m_UnpauseTimer = 0;
	}
}

bool CGameController::IsFriendlyFire(int ClientID1, int ClientID2)
{
	if(ClientID1 == ClientID2)
		return false;

	if(IsTeamplay())
	{
		if(!GameServer()->m_apPlayers[ClientID1] || !GameServer()->m_apPlayers[ClientID2])
			return false;

		if(GameServer()->m_apPlayers[ClientID1]->GetTeam() == GameServer()->m_apPlayers[ClientID2]->GetTeam())
			return true;
	}

	return false;
}

bool CGameController::IsForceBalanced()
{
	if(m_ForceBalanced)
	{
		m_ForceBalanced = false;
		return true;
	}
	else
		return false;
}

bool CGameController::CanBeMovedOnBalance(int ClientID)
{
	return true;
}

void CGameController::Tick()
{
	if(m_GameOverTick != -1)
	{
		// game over.. wait for restart
		if(Server()->Tick() > m_GameOverTick+Server()->TickSpeed()*10)
		{
			CycleMap();
			StartRound();
			m_RoundCount++;
		}
	}
	else if(GameServer()->m_World.m_Paused && m_UnpauseTimer)
	{
		--m_UnpauseTimer;
		if(!m_UnpauseTimer)
			GameServer()->m_World.m_Paused = false;
	}

	if(m_GameOverTick == -1)
	{
		if(GameServer()->GetBotNum() < MAX_BOTS && m_BotDataInit)
			OnCreateBot();
	}

	// game is Paused
	if(GameServer()->m_World.m_Paused)
		++m_RoundStartTick;

	// check for inactive players
	if(g_Config.m_SvInactiveKickTime > 0)
	{
		for(int i = 0; i < MAX_PLAYERS; ++i)
		{
			if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS && !Server()->IsAuthed(i))
			{
				if(Server()->Tick() > GameServer()->m_apPlayers[i]->m_LastActionTick+g_Config.m_SvInactiveKickTime*Server()->TickSpeed()*60)
				{
					switch(g_Config.m_SvInactiveKick)
					{
					case 0:
						{
							// move player to spectator
							GameServer()->m_apPlayers[i]->SetTeam(TEAM_SPECTATORS);
						}
						break;
					case 1:
						{
							// move player to spectator if the reserved slots aren't filled yet, kick him otherwise
							int Spectators = 0;
							for(int j = 0; j < MAX_CLIENTS; ++j)
								if(GameServer()->m_apPlayers[j] && GameServer()->m_apPlayers[j]->GetTeam() == TEAM_SPECTATORS)
									++Spectators;
							if(Spectators >= g_Config.m_SvSpectatorSlots)
								Server()->Kick(i, "Kicked for inactivity");
							else
								GameServer()->m_apPlayers[i]->SetTeam(TEAM_SPECTATORS);
						}
						break;
					case 2:
						{
							// kick the player
							Server()->Kick(i, "Kicked for inactivity");
						}
					}
				}
			}
		}
	}

	DoWincheck();
}


bool CGameController::IsTeamplay() const
{
	return m_GameFlags&GAMEFLAG_TEAMS;
}

void CGameController::Snap(int SnappingClient)
{
	CNetObj_GameInfo *pGameInfoObj = (CNetObj_GameInfo *)Server()->SnapNewItem(NETOBJTYPE_GAMEINFO, 0, sizeof(CNetObj_GameInfo));
	if(!pGameInfoObj)
		return;

	pGameInfoObj->m_GameFlags = m_GameFlags;
	pGameInfoObj->m_GameStateFlags = 0;
	if(m_GameOverTick != -1)
		pGameInfoObj->m_GameStateFlags |= GAMESTATEFLAG_GAMEOVER;
	if(m_SuddenDeath)
		pGameInfoObj->m_GameStateFlags |= GAMESTATEFLAG_SUDDENDEATH;
	if(GameServer()->m_World.m_Paused)
		pGameInfoObj->m_GameStateFlags |= GAMESTATEFLAG_PAUSED;
	pGameInfoObj->m_RoundStartTick = m_RoundStartTick;
	pGameInfoObj->m_WarmupTimer = 0;

	pGameInfoObj->m_ScoreLimit = 0;
	pGameInfoObj->m_TimeLimit = 0;

	pGameInfoObj->m_RoundNum = 0;
	pGameInfoObj->m_RoundCurrent = 1;
	
	CNetObj_GameInfoEx* pGameInfoEx = (CNetObj_GameInfoEx*)Server()->SnapNewItem(NETOBJTYPE_GAMEINFOEX, 0, sizeof(CNetObj_GameInfoEx));
	if(!pGameInfoEx)
		return;

	pGameInfoEx->m_Flags = GAMEINFOFLAG_GAMETYPE_PLUS | GAMEINFOFLAG_ALLOW_EYE_WHEEL | GAMEINFOFLAG_ALLOW_HOOK_COLL | GAMEINFOFLAG_PREDICT_VANILLA;
	pGameInfoEx->m_Flags2 = GAMEINFOFLAG2_GAMETYPE_CITY | GAMEINFOFLAG2_ALLOW_X_SKINS | GAMEINFOFLAG2_HUD_DDRACE | GAMEINFOFLAG2_HUD_HEALTH_ARMOR | GAMEINFOFLAG2_HUD_AMMO;
	pGameInfoEx->m_Version = GAMEINFO_CURVERSION;
}

int CGameController::GetAutoTeam(int NotThisID)
{
	// this will force the auto balancer to work overtime aswell
	if(g_Config.m_DbgStress)
		return 0;

	int aNumplayers[2] = {0,0};
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(GameServer()->m_apPlayers[i] && i != NotThisID)
		{
			if(GameServer()->m_apPlayers[i]->GetTeam() >= TEAM_RED && GameServer()->m_apPlayers[i]->GetTeam() <= TEAM_BLUE)
				aNumplayers[GameServer()->m_apPlayers[i]->GetTeam()]++;
		}
	}

	int Team = 0;
	if(IsTeamplay())
		Team = aNumplayers[TEAM_RED] > aNumplayers[TEAM_BLUE] ? TEAM_BLUE : TEAM_RED;

	if(CanJoinTeam(Team, NotThisID))
		return Team;
	return -1;
}

bool CGameController::CanJoinTeam(int Team, int NotThisID)
{
	if(Team == TEAM_SPECTATORS || (GameServer()->m_apPlayers[NotThisID] && GameServer()->m_apPlayers[NotThisID]->GetTeam() != TEAM_SPECTATORS))
		return true;

	int aNumplayers[2] = {0,0};
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(GameServer()->m_apPlayers[i] && i != NotThisID)
		{
			if(GameServer()->m_apPlayers[i]->GetTeam() >= TEAM_RED && GameServer()->m_apPlayers[i]->GetTeam() <= TEAM_BLUE)
				aNumplayers[GameServer()->m_apPlayers[i]->GetTeam()]++;
		}
	}

	return (aNumplayers[0] + aNumplayers[1]) < Server()->MaxClients()-g_Config.m_SvSpectatorSlots;
}

bool CGameController::CanChangeTeam(CPlayer *pPlayer, int JoinTeam)
{
	int aT[2] = {0, 0};

	if (!IsTeamplay() || JoinTeam == TEAM_SPECTATORS)
		return true;

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		CPlayer *pP = GameServer()->m_apPlayers[i];
		if(pP && pP->GetTeam() != TEAM_SPECTATORS)
			aT[pP->GetTeam()]++;
	}

	// simulate what would happen if changed team
	aT[JoinTeam]++;
	if (pPlayer->GetTeam() != TEAM_SPECTATORS)
		aT[JoinTeam^1]--;

	// there is a player-difference of at least 2
	if(absolute(aT[0]-aT[1]) >= 2)
	{
		// player wants to join team with less players
		if ((aT[0] < aT[1] && JoinTeam == TEAM_RED) || (aT[0] > aT[1] && JoinTeam == TEAM_BLUE))
			return true;
		else
			return false;
	}
	else
		return true;
}

void CGameController::DoWincheck()
{
}

int CGameController::ClampTeam(int Team)
{
	if(Team < 0)
		return TEAM_SPECTATORS;
	if(IsTeamplay())
		return Team&1;
	return 0;
}

double CGameController::GetTime()
{
	return static_cast<double>(Server()->Tick() - m_RoundStartTick)/Server()->TickSpeed();
}

void CGameController::ShowInventory(int ClientID)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientID];

	if(!pPlayer)
		return;

	if(!pPlayer->GetCharacter())
		return;

	const char *pLanguageCode = pPlayer->GetLanguage();

	CInventory *pData = GameServer()->Item()->GetInventory(ClientID);
	std::string Buffer;
	Buffer.clear();

	Buffer.append("===");
	Buffer.append(GameServer()->Localize(pLanguageCode, "Inventory"));
	Buffer.append("===");
	Buffer.append("\n");
	
	for(int i = 0; i < pData->m_Datas.size();i ++)
	{
		Buffer.append(GameServer()->Localize(pLanguageCode, pData->m_Datas[i].m_aName));
		Buffer.append(": ");
		Buffer.append(format_int64_with_commas(',', pData->m_Datas[i].m_Num));
		Buffer.append("\n");
	}
	Buffer.append("\n");

	if(!pData->m_Datas.size())
		Buffer.append(GameServer()->Localize(pLanguageCode, "You don't have any things!"));
	GameServer()->SendMotd(ClientID, Buffer.c_str());
}

void CGameController::OnCreateBot()
{
	for(int i = BOT_CLIENTS_START; i < MAX_CLIENTS; i ++)
	{
		if(GameServer()->m_apPlayers[i]) continue;
		CBotData *Data = RandomBotData();
		GameServer()->CreateBot(i, Data);
	}
}

static void InitBotDataThread(void *pUser)
{
	lock_wait(BotDataLock);
	CGameController *pController = (CGameController *) pUser;
	// read file data into buffer
	const char *pFilename = "./data/json/bot.json";
	std::ifstream File(pFilename);

	if(!File.is_open())
	{
		dbg_msg("Bot", "can't open 'data/json/bot.json'");
		lock_unlock(BotDataLock);
		return;
	}

	// parse json data
	json Data = json::parse(File);

	json BotArray = Data["json"];
	if(BotArray.is_array())
	{
		for(unsigned i = 0; i < BotArray.size(); ++i)
		{
			CBotData *pData = new CBotData();
			str_copy(pData->m_aName, BotArray[i].value("name", "null").c_str());
			str_copy(pData->m_SkinName, BotArray[i].value("skin", "default").c_str());
			pData->m_BodyColor = BotArray[i].value("body_color", -1);
			pData->m_FeetColor = BotArray[i].value("feet_color", -1);
			pData->m_AttackProba = BotArray[i].value("attack_proba", 20);
			pData->m_SpawnProba = BotArray[i].value("spawn_proba", 100);
			pData->m_TeamDamage = BotArray[i].value("teamdamage", 0);
			pData->m_Gun = BotArray[i].value("gun", 0);
			pData->m_Hammer = BotArray[i].value("hammer", 0);
			pData->m_Hook = BotArray[i].value("hook", 0);
			
			json DropsArray = BotArray[i]["drops"];
			if(DropsArray.is_array())
			{
				for(unsigned j = 0;j < DropsArray.size();j++)
				{
					CBotDropData *pDropData = new CBotDropData();
					str_copy(pDropData->m_ItemName, DropsArray[j].value("name", " ").c_str());
					pDropData->m_DropProba = DropsArray[j].value("proba", 0);
					pDropData->m_MinNum = DropsArray[j].value("minnum", 0);
					pDropData->m_MaxNum = DropsArray[j].value("maxnum", 0);
					pData->m_Drops.add(*pDropData);
				}
			}

			pController->m_BotDatas.add(*pData);
		}
	}
	pController->m_BotDataInit = true;
	lock_unlock(BotDataLock);
}

void CGameController::InitBotData()
{
	void *thread = thread_init(InitBotDataThread, this);
	thread_detach(thread);
}

CBotData *CGameController::RandomBotData()
{
	CBotData *pData;
	int RandomID;
	do
	{
		RandomID = random_int(0, m_BotDatas.size()-1);
	}
	while(random_int(1, 100) > m_BotDatas[RandomID].m_SpawnProba);
	pData = &m_BotDatas[RandomID];
	return pData;
}	

void CGameController::CreatePickup(vec2 Pos, vec2 Dir, CBotData BotData)
{
	for(int i = 0;i < BotData.m_Drops.size();i++)
	{
		if(random_int(1, 100) <= BotData.m_Drops[i].m_DropProba)
		{
			int PickupNum = random_int(BotData.m_Drops[i].m_MinNum, BotData.m_Drops[i].m_MaxNum);
			const char *pName = BotData.m_Drops[i].m_ItemName;
			dbg_msg(pName, "%d:%d", i, PickupNum);
			new CPickup(&GameServer()->m_World, Pos, Dir, pName, PickupNum);
		}
	}
}