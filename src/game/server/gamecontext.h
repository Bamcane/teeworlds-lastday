/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_GAMECONTEXT_H
#define GAME_SERVER_GAMECONTEXT_H

#include <engine/server.h>
#include <engine/storage.h>
#include <engine/console.h>
#include <engine/shared/memheap.h>

#include <teeuniverses/components/localization.h>

#include <game/layers.h>
#include <game/voting.h>

#include "eventhandler.h"
#include "gamecontroller.h"
#include "gameworld.h"
#include "player.h"
#include "define.h"

#include <bitset>

#ifdef _MSC_VER
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#else
#include <stdint.h>
#endif

#include "gamemenu.h"
#include "lastday/item/item.h"
#include "lastday/accounts/postgresql.h"
/*
	Tick
		Game Context (CGameContext::tick)
			Game World (GAMEWORLD::tick)
				Reset world if requested (GAMEWORLD::reset)
				All entities in the world (ENTITY::tick)
				All entities in the world (ENTITY::tick_defered)
				Remove entities marked for deletion (GAMEWORLD::remove_entities)
			Game Controller (GAMECONTROLLER::tick)
			All players (CPlayer::tick)


	Snap
		Game Context (CGameContext::snap)
			Game World (GAMEWORLD::snap)
				All entities in the world (ENTITY::snap)
			Game Controller (GAMECONTROLLER::snap)
			Events handler (EVENT_HANDLER::snap)
			All players (CPlayer::snap)

*/
std::bitset<MAX_CLIENTS> const& CmaskAll();
std::bitset<MAX_CLIENTS> CmaskOne(int ClientID);
std::bitset<MAX_CLIENTS> CmaskAllExceptOne(int ClientID);

inline bool CmaskIsSet(std::bitset<MAX_CLIENTS> const& Mask, int ClientID) { return Mask[ClientID]; }

class CGameContext : public IGameServer
{
	IServer *m_pServer;
	IStorage *m_pStorage;
	class IConsole *m_pConsole;
	CLayers m_Layers;
	CCollision m_Collision;
	CNetObjHandler m_NetObjHandler;
	CTuningParams m_Tuning;
	CMenu *m_pMenu;
	CItemCore *m_pItem;
	CPostgresql *m_pPostgresql;

	static void ConsoleOutputCallback_Chat(const char *pLine, void *pUser);

	static void ConLanguage(IConsole::IResult *pResult, void *pUserData);
	static void ConAbout(IConsole::IResult *pResult, void *pUserData);
	static void ConTuneParam(IConsole::IResult *pResult, void *pUserData);
	static void ConTuneReset(IConsole::IResult *pResult, void *pUserData);
	static void ConTuneDump(IConsole::IResult *pResult, void *pUserData);
	static void ConPause(IConsole::IResult *pResult, void *pUserData);
	static void ConChangeMap(IConsole::IResult *pResult, void *pUserData);
	static void ConRestart(IConsole::IResult *pResult, void *pUserData);
	static void ConBroadcast(IConsole::IResult *pResult, void *pUserData);
	static void ConSay(IConsole::IResult *pResult, void *pUserData);
	static void ConSetTeam(IConsole::IResult *pResult, void *pUserData);
	static void ConAddVote(IConsole::IResult *pResult, void *pUserData);
	static void ConRemoveVote(IConsole::IResult *pResult, void *pUserData);
	static void ConForceVote(IConsole::IResult *pResult, void *pUserData);
	static void ConClearVotes(IConsole::IResult *pResult, void *pUserData);
	static void ConVote(IConsole::IResult *pResult, void *pUserData);
	static void ConMapRegenerate(IConsole::IResult *pResult, void *pUserData);

	static void ConchainSpecialMotdupdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);

	static void ConMenu(IConsole::IResult *pResult, void *pUserData);
	static void ConEmote(IConsole::IResult *pResult, void *pUserData);

	static void ConRegister(IConsole::IResult *pResult, void *pUserData);
	static void ConLogin(IConsole::IResult *pResult, void *pUserData);

	static void MenuInventory(int ClientID, void *pUserData);
	static void MenuItem(int ClientID, void *pUserData);
	static void MenuSit(int ClientID, void *pUserData);


	CGameContext(int Resetting);
	void Construct(int Resetting);

	bool m_Resetting;

public:
	int m_ChatResponseTargetID;
	int m_ChatPrintCBIndex;

public:
	IServer *Server() const { return m_pServer; }
	IStorage *Storage() const { return m_pStorage; }
	class IConsole *Console() { return m_pConsole; }
	CCollision *Collision() { return &m_Collision; }
	CTuningParams *Tuning() { return &m_Tuning; }
	CMenu *Menu() { return m_pMenu; }
	CPostgresql *Postgresql() { return m_pPostgresql; }
	class CItemCore *Item() { return m_pItem; }
	class CLayers *Layers() override { return &m_Layers; }

	CGameContext();
	~CGameContext();

	void Clear();

	CEventHandler m_Events;
	CPlayer *m_apPlayers[MAX_CLIENTS];

	CGameController *m_pController;
	CGameWorld m_World;

	CTile *m_pTiles;

	// helper functions
	class CCharacter *GetPlayerChar(int ClientID);

	int m_LockTeams;

	// voting
	void StartVote(const char *pDesc, const char *pCommand, const char *pReason);
	void EndVote();
	void SendVoteSet(int ClientID);
	void SendVoteStatus(int ClientID, int Total, int Yes, int No);
	void AbortVoteKickOnDisconnect(int ClientID);

	int m_VoteCreator;
	int64 m_VoteCloseTime;
	bool m_VoteUpdate;
	int m_VotePos;
	char m_aVoteDescription[VOTE_DESC_LENGTH];
	char m_aVoteCommand[VOTE_CMD_LENGTH];
	char m_aVoteReason[VOTE_REASON_LENGTH];
	int m_NumVoteOptions;
	int m_VoteEnforce;
	enum
	{
		VOTE_ENFORCE_UNKNOWN=0,
		VOTE_ENFORCE_NO,
		VOTE_ENFORCE_YES,
	};
	CHeap *m_pVoteOptionHeap;
	CVoteOptionServer *m_pVoteOptionFirst;
	CVoteOptionServer *m_pVoteOptionLast;

	// helper functions
	void CreateDamageInd(vec2 Pos, float AngleMod, int Amount, std::bitset<MAX_CLIENTS> const& Mask=CmaskAll());
	void CreateExplosion(vec2 Pos, int Owner, int Weapon, bool NoDamage, std::bitset<MAX_CLIENTS> const& Mask=CmaskAll());
	void CreateHammerHit(vec2 Pos, std::bitset<MAX_CLIENTS> const& Mask=CmaskAll());
	void CreatePlayerSpawn(vec2 Pos, std::bitset<MAX_CLIENTS> const& Mask= CmaskAll());
	void CreateDeath(vec2 Pos, int Who, std::bitset<MAX_CLIENTS> const& Mask=CmaskAll());
	void CreateSound(vec2 Pos, int Sound, std::bitset<MAX_CLIENTS> const& Mask=CmaskAll());
	void CreateSoundGlobal(int Sound, int Target=-1);


	enum
	{
		CHAT_ALL=-2,
		CHAT_SPEC=-1,
		CHAT_RED=0,
		CHAT_BLUE=1
	};

	void SendMenuChat(int To, const char *pText);
	void SendMenuChat_Locazition(int To, const char *pText, ...);
	// network
	void SendMotd(int To, const char* pText);
	void SendChatTarget(int To, const char *pText);
	void SendChatTarget_Locazition(int To, const char *pText, ...);
	void SendChat(int ClientID, int Team, const char *pText);
	void SendEmoticon(int ClientID, int Emoticon);
	void SendWeaponPickup(int ClientID, int Weapon);
	void SendBroadcast(const char *pText, int ClientID);
	void SendBroadcast_VL(const char *pText, int ClientID, ...);
	void SetClientLanguage(int ClientID, const char *pLanguage);

	const char* Localize(const char *pLanguageCode, const char* pText) const;



	//
	void CheckPureTuning();
	void SendTuningParams(int ClientID);

	void OnMenuOptionsInit();

	// engine events
	void OnInit() override;
	void OnConsoleInit() override;
	void OnShutdown() override;

	void OnTick() override;
	void OnPreSnap() override;
	void OnSnap(int ClientID) override;
	void OnPostSnap() override;

	void OnMessage(int MsgID, CUnpacker *pUnpacker, int ClientID) override;

	void OnClientConnected(int ClientID) override;
	void OnClientEnter(int ClientID) override;
	void OnClientDrop(int ClientID, const char *pReason) override;
	void OnClientDirectInput(int ClientID, void *pInput) override;
	void OnClientPredictedInput(int ClientID, void *pInput) override;

	bool IsClientReady(int ClientID) override;
	bool IsClientPlayer(int ClientID) override;
	int GetClientVersion(int ClientID) const;

	void OnSetAuthed(int ClientID,int Level) override;
	
	const char *GameType() override;
	const char *Version() override;
	const char *NetVersion() override;
	
	void OnUpdatePlayerServerInfo(char *aBuf, int BufSize, int ID) override;

	// MakeItem
	void MakeItem(int ClientID, const char *pItemName);

	//Bot Start
	int GetBotNum() const;
	void OnBotDead(int ClientID);
	void CreateBot(int ClientID, CBotData *BotPower);

	//Bot END
};

#endif
