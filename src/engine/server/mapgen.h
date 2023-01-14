#ifndef ENGINE_SERVER_MAPCONVERTER_H
#define ENGINE_SERVER_MAPCONVERTER_H

#include <base/tl/array.h>
#include <engine/storage.h>
#include <engine/map.h>
#include <engine/console.h>
#include <engine/shared/datafile.h>
#include <game/mapitems.h>
#include <game/gamecore.h>
#include <engine/shared/imageinfo.h>

class CMapGen
{
protected:
	IStorage *m_pStorage;
	IConsole *m_pConsole;
	CDataFileWriter m_DataFile;
	
	CTile* m_pBackGroundTiles;
	CTile* m_pGameTiles;
	CTile* m_pHookableTiles;
	CTile* m_pUnhookableTiles;
	CTile* m_pDoodadsTiles;
	
	int m_NumGroups;
	int m_NumLayers;
	int m_NumImages;
	int m_NumEnvs;

protected:	
	IStorage* Storage() { return m_pStorage; };
	IConsole* Console() { return m_pConsole; };
	
	void InitQuad(CQuad* pQuad);
	void InitQuad(CQuad* pQuad, vec2 Pos, vec2 Size);
	void AddTile(CTile *pTile, const char *LayerName, int Image);
	void AddGameTile(CTile *pTile);

	void GenerateBackground();
	void GenerateBackgroundTile();
	void GenerateGameLayer();
	void GenerateDoodadsLayer();
	void GenerateHookableLayer();
	void GenerateUnhookableLayer();

	// auto map
	int LoadRules(const char *pImageName);
	void Proceed(CTile *pTiles, int ConfigID);

	struct CPosRule
	{
		int m_X;
		int m_Y;
		int m_Value;
		bool m_IndexValue;

		enum
		{
			EMPTY=0,
			FULL
		};
	};

	struct CIndexRule
	{
		int m_ID;
		array<CPosRule> m_aRules;
		int m_Flag;
		int m_RandomValue;
		int m_YDivisor;
		int m_YRemainder;
		bool m_BaseTile;
	};

	struct CConfiguration
	{
		array<CIndexRule> m_aIndexRules;
		char m_aName[128];
	};

	class CAirTile
	{
	public:
		CAirTile() {};
		CAirTile(int x, int y) { m_x = x; m_y = y; m_IsVisited = 0;}
		int m_x;
		int m_y;
		bool m_IsVisited;
	};
	
	class CArea
	{
	public:
		CArea() {};
		array<CAirTile> m_Tiles;
	};

	array<CConfiguration> m_lConfigs;

	void InitState();
	
	void AddImageQuad(const char* pName, int ImageID, int GridX, int GridY, int X, int Y, int Width, int Height, vec2 Pos, vec2 Size, vec4 Color, int Env);
	
	int LoadPNG(CImageInfo *pImg, const char *pFilename);
	int AddExternalImage(const char* pImageName, int Width, int Height);
	int AddEmbeddedImage(const char* pImageName, int Width, int Height);
	
	void GenerateMap();

public:
	CMapGen(IStorage *pStorage, IConsole* pConsole);
	~CMapGen();

	bool CreateMap(const char* pFilename);
};

#endif
