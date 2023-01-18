#ifndef GAME_SERVER_LASTDAY_ACCOUNTS_DATACORE_H
#define GAME_SERVER_LASTDAY_ACCOUNTS_DATACORE_H

#define MAX_ACCOUNTS_NAME_LENTH 32
#define MIN_ACCOUNTS_NAME_LENTH 8
#define MAX_ACCOUNTS_PASSWORD_LENTH 16
#define MIN_ACCOUNTS_PASSWORD_LENTH 6

class CPostgresql
{
public:
	
    CPostgresql(class CGameContext *pGameServer);

	void Init();
	void CreateRegisterThread(const char *pUserName, const char *pPassword, int ClientID);
	void CreateLoginThread(const char *pUserName, const char *pPassword, int ClientID);
	void CreateUpdateItemThread(int ClientID, const char *pItemName, int Num);
	void CreateSyncItemThread(int ClientID);
	void CreateClearItemThread(int ClientID);
};

class CTempAccountsData
{
public:
	CTempAccountsData(){}
	char Name[MAX_ACCOUNTS_NAME_LENTH];
	char Password[MAX_ACCOUNTS_PASSWORD_LENTH];
	int ClientID;
};

class CTempItemData
{
public:
	char Name[128];
	int Num;
	int ClientID;
	int UserID;
	CTempItemData(){ UserID = -1; }
};

#endif