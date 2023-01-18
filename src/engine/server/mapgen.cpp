#include <engine/server/mapgen.h>
#include <engine/shared/config.h>
#include <engine/shared/linereader.h>

#include <game/layers.h>
#include <game/mapitems.h>

#include <engine/external/open-simplex-noise/open-simplex-noise.h>
#include <engine/external/pnglite/pnglite.h>

#include <base/color.h>

CMapGen::CMapGen(IStorage *pStorage, IConsole* pConsole) :
	m_pStorage(pStorage),
	m_pConsole(pConsole),
	m_pBackGroundTiles(0),
	m_pGameTiles(0),
	m_pDoodadsTiles(0),
	m_pHookableTiles(0),
	m_pUnhookableTiles(0)
{
	
}

CMapGen::~CMapGen()
{
	if(m_pBackGroundTiles)
		delete[] m_pBackGroundTiles;
	if(m_pGameTiles)
		delete[] m_pGameTiles;
	if(m_pDoodadsTiles)
		delete[] m_pDoodadsTiles;
	if(m_pHookableTiles)
		delete[] m_pHookableTiles;
	if(m_pUnhookableTiles)
		delete[] m_pUnhookableTiles;
}

void CMapGen::InitQuad(CQuad* pQuad)
{
	for (int i=0; i<5; i++) {
		pQuad->m_aPoints[i].x = 0;
		pQuad->m_aPoints[i].y = 0;
	}
	pQuad->m_aColors[0].r = pQuad->m_aColors[1].r = 255;
	pQuad->m_aColors[0].g = pQuad->m_aColors[1].g = 255;
	pQuad->m_aColors[0].b = pQuad->m_aColors[1].b = 255;
	pQuad->m_aColors[0].a = pQuad->m_aColors[1].a = 255;
	pQuad->m_aColors[2].r = pQuad->m_aColors[3].r = 255;
	pQuad->m_aColors[2].g = pQuad->m_aColors[3].g = 255;
	pQuad->m_aColors[2].b = pQuad->m_aColors[3].b = 255;
	pQuad->m_aColors[2].a = pQuad->m_aColors[3].a = 255;
	pQuad->m_aTexcoords[0].x = 0;
	pQuad->m_aTexcoords[0].y = 0;
	pQuad->m_aTexcoords[1].x = 1<<10;
	pQuad->m_aTexcoords[1].y = 0;
	pQuad->m_aTexcoords[2].x = 0;
	pQuad->m_aTexcoords[2].y = 1<<10;
	pQuad->m_aTexcoords[3].x = 1<<10;
	pQuad->m_aTexcoords[3].y = 1<<10;
	pQuad->m_PosEnv = -1;
	pQuad->m_PosEnvOffset = 0;
	pQuad->m_ColorEnv = -1;
	pQuad->m_ColorEnvOffset = 0;
}

void CMapGen::InitQuad(CQuad* pQuad, vec2 Pos, vec2 Size)
{
	int X0 = f2fx(Pos.x-Size.x/2.0f);
	int X1 = f2fx(Pos.x+Size.x/2.0f);
	int XC = f2fx(Pos.x);
	int Y0 = f2fx(Pos.y-Size.y/2.0f);
	int Y1 = f2fx(Pos.y+Size.y/2.0f);
	int YC = f2fx(Pos.y);
	
	InitQuad(pQuad);
	pQuad->m_aPoints[0].x = pQuad->m_aPoints[2].x = X0;
	pQuad->m_aPoints[1].x = pQuad->m_aPoints[3].x = X1;
	pQuad->m_aPoints[0].y = pQuad->m_aPoints[1].y = Y0;
	pQuad->m_aPoints[2].y = pQuad->m_aPoints[3].y = Y1;
	pQuad->m_aPoints[4].x = XC;
	pQuad->m_aPoints[4].y = YC;
}

void CMapGen::AddImageQuad(const char* pName, int ImageID, int GridX, int GridY, int X, int Y, int Width, int Height, vec2 Pos, vec2 Size, vec4 Color, int Env)
{
	array<CQuad> aQuads;
	CQuad Quad;
	
	float StepX = 1.0f/GridX;
	float StepY = 1.0f/GridY;
	
	InitQuad(&Quad, Pos, Size);
	Quad.m_ColorEnv = Env;
	Quad.m_aTexcoords[0].x = Quad.m_aTexcoords[2].x = f2fx(StepX * X);
	Quad.m_aTexcoords[1].x = Quad.m_aTexcoords[3].x = f2fx(StepX * (X + Width));
	Quad.m_aTexcoords[0].y = Quad.m_aTexcoords[1].y = f2fx(StepY * Y);
	Quad.m_aTexcoords[2].y = Quad.m_aTexcoords[3].y = f2fx(StepY * (Y + Height));
	Quad.m_aColors[0].r = Quad.m_aColors[1].r = Quad.m_aColors[2].r = Quad.m_aColors[3].r = Color.r*255.0f;
	Quad.m_aColors[0].g = Quad.m_aColors[1].g = Quad.m_aColors[2].g = Quad.m_aColors[3].g = Color.g*255.0f;
	Quad.m_aColors[0].b = Quad.m_aColors[1].b = Quad.m_aColors[2].b = Quad.m_aColors[3].b = Color.b*255.0f;
	Quad.m_aColors[0].a = Quad.m_aColors[1].a = Quad.m_aColors[2].a = Quad.m_aColors[3].a = Color.a*255.0f;
	aQuads.add(Quad);
	
	CMapItemLayerQuads Item;
	Item.m_Version = 2;
	Item.m_Layer.m_Flags = 0;
	Item.m_Layer.m_Type = LAYERTYPE_QUADS;
	Item.m_Image = ImageID;
	Item.m_NumQuads = aQuads.size();
	StrToInts(Item.m_aName, sizeof(Item.m_aName)/sizeof(int), pName);
	Item.m_Data = m_DataFile.AddDataSwapped(aQuads.size()*sizeof(CQuad), aQuads.base_ptr());
	
	m_DataFile.AddItem(MAPITEMTYPE_LAYER, m_NumLayers++, sizeof(Item), &Item);
}

void CMapGen::InitState()
{
	m_NumGroups = 0;
	m_NumLayers = 0;
	m_NumImages = 0;
	m_NumEnvs = 0;
	m_lConfigs.clear();
}

int CMapGen::LoadPNG(CImageInfo *pImg, const char *pFilename)
{
	char aCompleteFilename[512];
	unsigned char *pBuffer;
	png_t Png; // ignore_convention

	// open file for reading
	png_init(0,0); // ignore_convention

	IOHANDLE File = m_pStorage->OpenFile(pFilename, IOFLAG_READ, IStorage::TYPE_ALL, aCompleteFilename, sizeof(aCompleteFilename));
	if(File)
		io_close(File);
	else
	{
		dbg_msg("game/png", "failed to open file. filename='%s'", pFilename);
		return 0;
	}

	int Error = png_open_file(&Png, aCompleteFilename); // ignore_convention
	if(Error != PNG_NO_ERROR)
	{
		dbg_msg("game/png", "failed to open file. filename='%s'", aCompleteFilename);
		if(Error != PNG_FILE_ERROR)
			png_close_file(&Png); // ignore_convention
		return 0;
	}

	if(Png.depth != 8 || (Png.color_type != PNG_TRUECOLOR && Png.color_type != PNG_TRUECOLOR_ALPHA)) // ignore_convention
	{
		dbg_msg("game/png", "invalid format. filename='%s'", aCompleteFilename);
		png_close_file(&Png); // ignore_convention
		return 0;
	}

	pBuffer = (unsigned char *)mem_alloc(Png.width * Png.height * Png.bpp, 1); // ignore_convention
	png_get_data(&Png, pBuffer); // ignore_convention
	png_close_file(&Png); // ignore_convention

	pImg->m_Width = Png.width; // ignore_convention
	pImg->m_Height = Png.height; // ignore_convention
	if(Png.color_type == PNG_TRUECOLOR) // ignore_convention
		pImg->m_Format = CImageInfo::FORMAT_RGB;
	else if(Png.color_type == PNG_TRUECOLOR_ALPHA) // ignore_convention
		pImg->m_Format = CImageInfo::FORMAT_RGBA;
	pImg->m_pData = pBuffer;
	return 1;
}

void FreePNG(CImageInfo *pImg)
{
	mem_free(pImg->m_pData);
	pImg->m_pData = nullptr;
}

int CMapGen::AddEmbeddedImage(const char* pImageName, int Width, int Height)
{
	CImageInfo img;
	CImageInfo *pImg = &img;

	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "data/mapgen/%s.png", pImageName);

	if (!LoadPNG(pImg, aBuf)) {
		return -1;
	}

	CMapItemImage Item;
	Item.m_Version = 1;

	Item.m_External = 0;
	Item.m_Width = Width;
	Item.m_Height = Height;
	Item.m_ImageName = m_DataFile.AddData(str_length((char*)pImageName)+1, (char*)pImageName);

	Item.m_Width = pImg->m_Width;
	Item.m_Height = pImg->m_Height;

	if(pImg->m_Format == CImageInfo::FORMAT_RGB)
	{
		// Convert to RGBA
		unsigned char *pDataRGBA = (unsigned char *)malloc((size_t)Item.m_Width * Item.m_Height * 4);
		unsigned char *pDataRGB = (unsigned char *)pImg->m_pData;
		for(int i = 0; i < Item.m_Width * Item.m_Height; i++)
		{
			pDataRGBA[i * 4] = pDataRGB[i * 3];
			pDataRGBA[i * 4 + 1] = pDataRGB[i * 3 + 1];
			pDataRGBA[i * 4 + 2] = pDataRGB[i * 3 + 2];
			pDataRGBA[i * 4 + 3] = 255;
		}
		Item.m_ImageData = m_DataFile.AddData(Item.m_Width * Item.m_Height * 4, pDataRGBA);
		free(pDataRGBA);
	}
	else
	{
		Item.m_ImageData = m_DataFile.AddData(Item.m_Width * Item.m_Height * 4, pImg->m_pData);
	}
	m_DataFile.AddItem(MAPITEMTYPE_IMAGE, m_NumImages++, sizeof(Item), &Item);

	FreePNG(pImg);

	return m_NumImages-1;
}

int CMapGen::AddExternalImage(const char* pImageName, int Width, int Height)
{
	CMapItemImage Item;
	Item.m_Version = 1;
	Item.m_External = 1;
	Item.m_ImageData = -1;
	Item.m_ImageName = m_DataFile.AddData(str_length((char*)pImageName)+1, (char*)pImageName);
	Item.m_Width = Width;
	Item.m_Height = Height;
	m_DataFile.AddItem(MAPITEMTYPE_IMAGE, m_NumImages++, sizeof(CMapItemImage), &Item);
	
	return m_NumImages-1;
}

void CMapGen::GenerateGameLayer()
{
	int Width = g_Config.m_SvGeneratedMapWidth;
	int Height = g_Config.m_SvGeneratedMapHeight;

	CMapItemGroup Item;
	Item.m_Version = CMapItemGroup::CURRENT_VERSION;
	Item.m_ParallaxX = 100;
	Item.m_ParallaxY = 100;
	Item.m_OffsetX = 0;
	Item.m_OffsetY = 0;
	Item.m_StartLayer = m_NumLayers;
	Item.m_NumLayers = 1;
	Item.m_UseClipping = 0;
	Item.m_ClipX = 0;
	Item.m_ClipY = 0;
	Item.m_ClipW = 0;
	Item.m_ClipH = 0;
	StrToInts(Item.m_aName, sizeof(Item.m_aName)/sizeof(int), "Game");

	// create tiles
	m_pGameTiles = new CTile[Width*Height];
	
	// fiil tiles to solid
	for(int x = 0;x < Width; x ++)
	{
		for(int y = 0;y < Height; y ++)
		{
			m_pGameTiles[y*Width+x].m_Index = TILE_SOLID;
			m_pGameTiles[y*Width+x].m_Flags = 0;
			m_pGameTiles[y*Width+x].m_Reserved = 0;
			m_pGameTiles[y*Width+x].m_Skip = 0;
		}
	}
	
	struct osn_context *ctx;

	open_simplex_noise(random_int(40000, 55734), &ctx);
	// noise create nohook tiles
	for(int x = 0;x < Width; x ++)
	{
		for(int y = 0;y < Height; y ++)
		{
			if(x < 1 || x > Width-2 || y < 1 || y > Height-2)
			{
				continue;
			}
			double value = open_simplex_noise2(ctx, (double) x/24, (double) y/24) * 0.5 + 0.5;
			if(value < 0.2f && m_pGameTiles[y*Width+x].m_Index == TILE_SOLID)
			{
				m_pGameTiles[y*Width+x].m_Index = TILE_NOHOOK;
			}
		}
	}

	// noise create air tiles
	open_simplex_noise(random_int(100000, 107374), &ctx);
	for(int x = 0;x < Width; x ++)
	{
		for(int y = 0;y < Height; y ++)
		{
			double value = open_simplex_noise2(ctx, (double) x/16, (double) y/16) * 0.5 + 0.5;
			if(value < 0.5f && m_pGameTiles[y*Width+x].m_Index == TILE_SOLID)
			{
				m_pGameTiles[y*Width+x].m_Index = TILE_AIR;
			}
		}
	}
	

	// create border
	for(int x = 0;x < Width; x ++)
	{
		for(int y = 0;y < Height; y ++)
		{
			if(x <= 3 || x >= Width-4 || y <= 3 || y >= Height-4)
			{
				m_pGameTiles[y*Width+x].m_Index = TILE_SOLID;
			}
		}
	}

	// CloseMap
	CAirTile* TileList = new CAirTile[Width*Height];
	array<CAirTile*> AirList;
	array<CArea*> AreaList;

	for(int y = 0;y < Height;y ++)
	{
		for(int x = 0;x < Width;x ++)
		{
			TileList[y*Width+x] = *(new CAirTile(x, y));
			if(m_pGameTiles[y*Width+x].m_Index == TILE_AIR)
				AirList.add(new CAirTile(x, y));
		}
	}

	while (AirList.size() > 0)
	{
		if(AirList.size() == 1)
		{
			CArea area;
			area.m_Tiles.add(*AirList[0]);
			AreaList.add(&area);
			break;
		}else
		{
			CArea *area = new CArea();
			area->m_Tiles.add(*AirList[0]);
			for(int i = 0; i < area->m_Tiles.size();i ++)
			{
				int x = area->m_Tiles[i].m_x;
				int y = area->m_Tiles[i].m_y;
				TileList[y*Width+x].m_IsVisited = true;
				if(m_pGameTiles[y*Width+x+1].m_Index == TILE_AIR && TileList[y*Width+x+1].m_IsVisited == false)
				{
					TileList[y*Width+x+1].m_IsVisited = true;
					area->m_Tiles.add(TileList[y*Width+x+1]);
				}
				if(m_pGameTiles[y*Width+x-1].m_Index == TILE_AIR && TileList[y*Width+x-1].m_IsVisited == false)
				{
					TileList[y*Width+x-1].m_IsVisited = true;
					area->m_Tiles.add(TileList[y*Width+x-1]);
				}
				if(m_pGameTiles[(y-1)*Width+x].m_Index == TILE_AIR && TileList[(y-1)*Width+x].m_IsVisited == false)
				{
					TileList[(y-1)*Width+x].m_IsVisited = true;
					area->m_Tiles.add(TileList[(y-1)*Width+x]);
				}
				if(m_pGameTiles[(y+1)*Width+x].m_Index == TILE_AIR && TileList[(y+1)*Width+x].m_IsVisited == false)
				{
					TileList[(y-1)*Width+x].m_IsVisited = true;
					area->m_Tiles.add(TileList[(y+1)*Width+x]);
				}
			}
			for(int i = 0;i < area->m_Tiles.size();i++)
			{
				CAirTile pTile = area->m_Tiles[i];
				for(int j = 0;j < AirList.size();j++)
				{
					CAirTile *pAirTile = AirList[j];
					if(pTile.m_x == pAirTile->m_x && pTile.m_y == pAirTile->m_y)
					{
						AirList.remove_index(j);
						break;
					}
				}
		
			}
			AreaList.add(area);
		}
	}

	dbg_msg("mapgen", "game tiles clear area to 1", AreaList.size());
	while (AreaList.size() > 1)
	{
		if(!AreaList[1])
			break;

		dbg_msg("mapgen", "%d Area left", AreaList.size());
		int tempdis = Width;
		CAirTile Start, End;
		End = AreaList[1]->m_Tiles[AreaList[1]->m_Tiles.size() > 1 ? random_int(0, AreaList[1]->m_Tiles.size()-1) : 0];
		for(int i = 0;i < AreaList[0]->m_Tiles.size();i ++)
		{
			CAirTile pTile = AreaList[0]->m_Tiles[i];
			if(abs(pTile.m_x-End.m_x)+ abs(pTile.m_y - End.m_y) < tempdis)
			{
				tempdis = abs(pTile.m_x-End.m_x)+ abs(pTile.m_y - End.m_y);
				Start = pTile;
			}
			if(i + 2 < AreaList[0]->m_Tiles.size())
				i++;
		}
		if (Start.m_x < End.m_x)
		{
			for(int x = Start.m_x; x < End.m_x; x++)
			{
				m_pGameTiles[Start.m_y*Width+x-1].m_Index = 0;
				m_pGameTiles[Start.m_y*Width+x+1].m_Index = 0;
				m_pGameTiles[Start.m_y*Width+x].m_Index = 0;
				m_pGameTiles[(Start.m_y-1)*Width+x].m_Index = 0;
				m_pGameTiles[(Start.m_y+1)*Width+x].m_Index = 0;
			}
			if(Start.m_y < End.m_y)
			{
				for (int y = Start.m_y; y < End.m_y; y++)
				{
					m_pGameTiles[y*Width+End.m_x-1].m_Index = 0;
					m_pGameTiles[y*Width+End.m_x+1].m_Index = 0;
					m_pGameTiles[y*Width+End.m_x].m_Index = 0;
					m_pGameTiles[(y-1)*Width+End.m_x].m_Index = 0;
					m_pGameTiles[(y+1)*Width+End.m_x].m_Index = 0;
				}
			}
			else
			{
				for (int y = Start.m_y; y > End.m_y; y--)
				{
					m_pGameTiles[y*Width+End.m_x-1].m_Index = 0;
					m_pGameTiles[y*Width+End.m_x+1].m_Index = 0;
					m_pGameTiles[y*Width+End.m_x].m_Index = 0;
					m_pGameTiles[(y-1)*Width+End.m_x].m_Index = 0;
					m_pGameTiles[(y+1)*Width+End.m_x].m_Index = 0;
				}
			}
		}
		else
		{
			for (int x = Start.m_x; x > End.m_x; x--)
			{
				m_pGameTiles[Start.m_y*Width+x-1].m_Index = 0;
				m_pGameTiles[Start.m_y*Width+x+1].m_Index = 0;
				m_pGameTiles[Start.m_y*Width+x].m_Index = 0;
				m_pGameTiles[(Start.m_y-1)*Width+x].m_Index = 0;
				m_pGameTiles[(Start.m_y+1)*Width+x].m_Index = 0;
			}
			if (Start.m_y < End.m_y)
			{
				for (int y = Start.m_y; y < End.m_y; y++)
				{
					m_pGameTiles[y*Width+End.m_x-1].m_Index = 0;
					m_pGameTiles[y*Width+End.m_x+1].m_Index = 0;
					m_pGameTiles[y*Width+End.m_x].m_Index = 0;
					m_pGameTiles[(y-1)*Width+End.m_x].m_Index = 0;
					m_pGameTiles[(y+1)*Width+End.m_x].m_Index = 0;
				}
			}
			else
			{
				for (int y = Start.m_y; y > End.m_y; y--)
				{
					m_pGameTiles[y*Width+End.m_x-1].m_Index = 0;
					m_pGameTiles[y*Width+End.m_x+1].m_Index = 0;
					m_pGameTiles[y*Width+End.m_x].m_Index = 0;
					m_pGameTiles[(y-1)*Width+End.m_x].m_Index = 0;
					m_pGameTiles[(y+1)*Width+End.m_x].m_Index = 0;
				}
			}
		}
		for(int i = 0; i < AreaList[1]->m_Tiles.size();i++)
		{
			CAirTile pTile = AreaList[1]->m_Tiles[i];
			AreaList[0]->m_Tiles.add(pTile);
		}
		AreaList.remove_index(1);           
	}
	
	AddGameTile(m_pGameTiles);
	
	m_DataFile.AddItem(MAPITEMTYPE_GROUP, m_NumGroups++, sizeof(Item), &Item);

	if(TileList)
		delete[] TileList;
	AreaList.clear();
	AirList.clear();
} 

void CMapGen::GenerateBackgroundTile()
{
	int Width = g_Config.m_SvGeneratedMapWidth;
	int Height = g_Config.m_SvGeneratedMapHeight;

	CMapItemGroup Item;
	Item.m_Version = CMapItemGroup::CURRENT_VERSION;
	Item.m_ParallaxX = 100;
	Item.m_ParallaxY = 100;
	Item.m_OffsetX = 0;
	Item.m_OffsetY = 0;
	Item.m_StartLayer = m_NumLayers;
	Item.m_NumLayers = 1;
	Item.m_UseClipping = 0;
	Item.m_ClipX = 0;
	Item.m_ClipY = 0;
	Item.m_ClipW = 0;
	Item.m_ClipH = 0;
	StrToInts(Item.m_aName, sizeof(Item.m_aName)/sizeof(int), "Tiles");

	int Image = AddExternalImage("grass_main", 1024, 1024);
	int Rule = LoadRules("grass_main");

	m_pBackGroundTiles = new CTile[Width*Height];
	struct osn_context *ctx;
	open_simplex_noise(random_int(90000, 97374), &ctx);
	for(int x = 0;x < Width; x ++)
	{
		for(int y = 0;y < Height; y ++)
		{
			double value = open_simplex_noise2(ctx, (double) x/32, (double) y/32) * 0.5 + 0.5;
			m_pBackGroundTiles[y*Width+x].m_Flags = 0;
			m_pBackGroundTiles[y*Width+x].m_Reserved = 0;
			m_pBackGroundTiles[y*Width+x].m_Skip = 0;
			if(value < 0.45f)
			{
				m_pBackGroundTiles[y*Width+x].m_Index = 1;
			}else m_pBackGroundTiles[y*Width+x].m_Index = 0;
		}
	}
	Proceed(m_pBackGroundTiles, Rule);
	
	CMapItemLayerTilemap LayerItem;
	LayerItem.m_Version = LayerItem.m_Layer.m_Version = 3;
	LayerItem.m_Layer.m_Flags = 0;
	LayerItem.m_Layer.m_Type = LAYERTYPE_TILES;
	LayerItem.m_Color.r = 100;
	LayerItem.m_Color.g = 100;
	LayerItem.m_Color.b = 100;
	LayerItem.m_Color.a = 255;
	LayerItem.m_ColorEnv = -1;
	LayerItem.m_ColorEnvOffset = 0;
	LayerItem.m_Width = Width;
	LayerItem.m_Height = Height;
	LayerItem.m_Flags = 0;
	LayerItem.m_Image = Image;

	LayerItem.m_Data = m_DataFile.AddData(Width*Height*sizeof(CTile), m_pBackGroundTiles);
	StrToInts(LayerItem.m_aName, sizeof(LayerItem.m_aName)/sizeof(int), "Background");
	m_DataFile.AddItem(MAPITEMTYPE_LAYER, m_NumLayers++, sizeof(LayerItem), &LayerItem);
	
	m_DataFile.AddItem(MAPITEMTYPE_GROUP, m_NumGroups++, sizeof(Item), &Item);
}

void CMapGen::GenerateBackground()
{
	int Width = g_Config.m_SvGeneratedMapWidth;
	int Height = g_Config.m_SvGeneratedMapHeight;

	CMapItemGroup Item;
	Item.m_Version = CMapItemGroup::CURRENT_VERSION;
	Item.m_ParallaxX = 0;
	Item.m_ParallaxY = 0;
	Item.m_OffsetX = 0;
	Item.m_OffsetY = 0;
	Item.m_StartLayer = m_NumLayers;
	Item.m_NumLayers = 1;
	Item.m_UseClipping = 0;
	Item.m_ClipX = 0;
	Item.m_ClipY = 0;
	Item.m_ClipW = 0;
	Item.m_ClipH = 0;
	StrToInts(Item.m_aName, sizeof(Item.m_aName)/sizeof(int), "Quad");

	array<CQuad> aQuads;
	CQuad Quad;
	{
		InitQuad(&Quad, vec2(0, 0), vec2(1600, 1200));
		Quad.m_aColors[0].r = Quad.m_aColors[1].r = 0;
		Quad.m_aColors[0].g = Quad.m_aColors[1].g = 100;
		Quad.m_aColors[0].b = Quad.m_aColors[1].b = 200;
		Quad.m_aColors[2].r = Quad.m_aColors[3].r = 156;
		Quad.m_aColors[2].g = Quad.m_aColors[3].g = 168;
		Quad.m_aColors[2].b = Quad.m_aColors[3].b = 161;
		aQuads.add(Quad);
	}
	{
		InitQuad(&Quad, vec2(0, 0), vec2(1600, 1200));
		Quad.m_aColors[0].r = Quad.m_aColors[1].r = 0;
		Quad.m_aColors[0].g = Quad.m_aColors[1].g = 0;
		Quad.m_aColors[0].b = Quad.m_aColors[1].b = 0;
		Quad.m_aColors[0].a = Quad.m_aColors[1].a = 50;
		Quad.m_aColors[2].r = Quad.m_aColors[3].r = 0;
		Quad.m_aColors[2].g = Quad.m_aColors[3].g = 0;
		Quad.m_aColors[2].b = Quad.m_aColors[3].b = 0;
		Quad.m_aColors[2].a = Quad.m_aColors[3].a = 50;
		aQuads.add(Quad);
	}

	CMapItemLayerQuads LayerItem;

	LayerItem.m_Image = -1;
	LayerItem.m_NumQuads = aQuads.size();
	LayerItem.m_Version = LayerItem.m_Layer.m_Version = 3;
	LayerItem.m_Layer.m_Flags = 0;
	LayerItem.m_Layer.m_Type = LAYERTYPE_QUADS;

	StrToInts(LayerItem.m_aName, sizeof(LayerItem.m_aName)/sizeof(int), "Quad");
	LayerItem.m_Data = m_DataFile.AddDataSwapped(aQuads.size()*sizeof(CQuad), aQuads.base_ptr());
				
	m_DataFile.AddItem(MAPITEMTYPE_LAYER, m_NumLayers++, sizeof(LayerItem), &LayerItem);
		
	m_DataFile.AddItem(MAPITEMTYPE_GROUP, m_NumGroups++, sizeof(Item), &Item);
}

void CMapGen::GenerateDoodadsLayer()
{
	int Width = g_Config.m_SvGeneratedMapWidth;
	int Height = g_Config.m_SvGeneratedMapHeight;
	CMapItemGroup Item;
	Item.m_Version = CMapItemGroup::CURRENT_VERSION;
	Item.m_ParallaxX = 100;
	Item.m_ParallaxY = 100;
	Item.m_OffsetX = 0;
	Item.m_OffsetY = 0;
	Item.m_StartLayer = m_NumLayers;
	Item.m_NumLayers = 1;
	Item.m_UseClipping = 0;
	Item.m_ClipX = 0;
	Item.m_ClipY = 0;
	Item.m_ClipW = 0;
	Item.m_ClipH = 0;
	StrToInts(Item.m_aName, sizeof(Item.m_aName)/sizeof(int), "Doodads");

	int Image = AddExternalImage("jungle_doodads", 1024, 1024);

	m_pDoodadsTiles = new CTile[Width*Height];
	for(int x = 0;x < Width;x ++)
	{
		for(int y = 0;y < Height;y ++)
		{
			m_pDoodadsTiles[y*Width+x].m_Index = 0;
			m_pDoodadsTiles[y*Width+x].m_Flags = 0;
			m_pDoodadsTiles[y*Width+x].m_Reserved = 0;
			m_pDoodadsTiles[y*Width+x].m_Skip = 0;
		}
	}

	for(int y = 0;y < Height-3;y ++)
	{
		for(int x = 0;x < Width-9;x ++)
		{
			if(m_pDoodadsTiles[(y+1)*Width+x].m_Index != 0
				|| m_pDoodadsTiles[(y+1)*Width+x+1].m_Index != 0
				|| m_pDoodadsTiles[(y+1)*Width+x+2].m_Index != 0
				|| m_pDoodadsTiles[(y+1)*Width+x+3].m_Index != 0
				|| m_pDoodadsTiles[(y+1)*Width+x+4].m_Index != 0
				|| m_pDoodadsTiles[(y+1)*Width+x+5].m_Index != 0
				|| m_pDoodadsTiles[(y+1)*Width+x+6].m_Index != 0
				|| m_pDoodadsTiles[(y+1)*Width+x+7].m_Index != 0
				|| m_pDoodadsTiles[(y+1)*Width+x+8].m_Index != 0)
				continue;
			if(m_pDoodadsTiles[(y+2)*Width+x].m_Index != 0
				|| m_pDoodadsTiles[(y+2)*Width+x+1].m_Index != 0
				|| m_pDoodadsTiles[(y+2)*Width+x+2].m_Index != 0
				|| m_pDoodadsTiles[(y+2)*Width+x+3].m_Index != 0
				|| m_pDoodadsTiles[(y+2)*Width+x+4].m_Index != 0
				|| m_pDoodadsTiles[(y+2)*Width+x+5].m_Index != 0
				|| m_pDoodadsTiles[(y+2)*Width+x+6].m_Index != 0
				|| m_pDoodadsTiles[(y+2)*Width+x+7].m_Index != 0
				|| m_pDoodadsTiles[(y+2)*Width+x+8].m_Index != 0)
				continue;
			if(m_pDoodadsTiles[(y+3)*Width+x].m_Index != 0
				|| m_pDoodadsTiles[(y+3)*Width+x+1].m_Index != 0
				|| m_pDoodadsTiles[(y+3)*Width+x+2].m_Index != 0
				|| m_pDoodadsTiles[(y+3)*Width+x+3].m_Index != 0
				|| m_pDoodadsTiles[(y+3)*Width+x+4].m_Index != 0
				|| m_pDoodadsTiles[(y+3)*Width+x+5].m_Index != 0
				|| m_pDoodadsTiles[(y+3)*Width+x+6].m_Index != 0
				|| m_pDoodadsTiles[(y+3)*Width+x+7].m_Index != 0
				|| m_pDoodadsTiles[(y+3)*Width+x+8].m_Index != 0)
				continue;

			if(m_pGameTiles[(y+1)*Width+x].m_Index == TILE_AIR && 
				m_pGameTiles[(y+1)*Width+x+1].m_Index == TILE_AIR 
					&& m_pGameTiles[(y+1)*Width+x+2].m_Index == TILE_AIR
					&& m_pGameTiles[(y+1)*Width+x+3].m_Index == TILE_AIR
					&& m_pGameTiles[(y+1)*Width+x+4].m_Index == TILE_AIR
					&& m_pGameTiles[(y+1)*Width+x+5].m_Index == TILE_AIR
					&& m_pGameTiles[(y+1)*Width+x+6].m_Index == TILE_AIR
					&& m_pGameTiles[(y+1)*Width+x+7].m_Index == TILE_AIR
					&& m_pGameTiles[(y+1)*Width+x+8].m_Index == TILE_AIR)
			{
				if(m_pGameTiles[(y+4)*Width+x].m_Index != TILE_AIR && 
					m_pGameTiles[(y+4)*Width+x+1].m_Index != TILE_AIR 
						&& m_pGameTiles[(y+4)*Width+x+2].m_Index != TILE_AIR
						&& m_pGameTiles[(y+4)*Width+x+3].m_Index != TILE_AIR
						&& m_pGameTiles[(y+4)*Width+x+4].m_Index != TILE_AIR
						&& m_pGameTiles[(y+4)*Width+x+5].m_Index != TILE_AIR
						&& m_pGameTiles[(y+4)*Width+x+6].m_Index != TILE_AIR
						&& m_pGameTiles[(y+4)*Width+x+7].m_Index != TILE_AIR
						&& m_pGameTiles[(y+4)*Width+x+8].m_Index != TILE_AIR)
				{
					for(int i = 0;i < 9;i++)
					{
						for(int j = 0;j < 3;j++)
						{
							m_pDoodadsTiles[(y+1+j)*Width+x+i].m_Index = 6+16*j+i;
							m_pDoodadsTiles[(y+1+j)*Width+x+i].m_Flags = 0;
						}
					}
				}
			}
		}
	}
	AddTile(m_pDoodadsTiles, "Doodads", Image);
	
	m_DataFile.AddItem(MAPITEMTYPE_GROUP, m_NumGroups++, sizeof(Item), &Item);
}

void CMapGen::GenerateHookableLayer()
{
	int Width = g_Config.m_SvGeneratedMapWidth;
	int Height = g_Config.m_SvGeneratedMapHeight;

	CMapItemGroup Item;
	Item.m_Version = CMapItemGroup::CURRENT_VERSION;
	Item.m_ParallaxX = 100;
	Item.m_ParallaxY = 100;
	Item.m_OffsetX = 0;
	Item.m_OffsetY = 0;
	Item.m_StartLayer = m_NumLayers;
	Item.m_NumLayers = 1;
	Item.m_UseClipping = 0;
	Item.m_ClipX = 0;
	Item.m_ClipY = 0;
	Item.m_ClipW = 0;
	Item.m_ClipH = 0;
	StrToInts(Item.m_aName, sizeof(Item.m_aName)/sizeof(int), "Tiles");

	int ImageHookable = AddEmbeddedImage("grass_main_0.7", 1024, 1024);
	int RuleHookable = LoadRules("grass_main_0.7");

	m_pHookableTiles = new CTile[Width*Height];
	for(int x = 0;x < Width;x ++)
	{
		for(int y = 0;y < Height;y ++)
		{
			m_pHookableTiles[y*Width+x].m_Flags = 0;
			m_pHookableTiles[y*Width+x].m_Reserved = 0;
			m_pHookableTiles[y*Width+x].m_Skip = 0;
			if(m_pGameTiles[y*Width+x].m_Index == TILE_SOLID || m_pGameTiles[y*Width+x].m_Index == TILE_NOHOOK)
			{
				m_pHookableTiles[y*Width+x].m_Index = 1;
			}else 
			{
				m_pHookableTiles[y*Width+x].m_Index = 0;
			}
		}
	}
	Proceed(m_pHookableTiles, RuleHookable);
	AddTile(m_pHookableTiles, "Hookable", ImageHookable);
	
	m_DataFile.AddItem(MAPITEMTYPE_GROUP, m_NumGroups++, sizeof(Item), &Item);
}

void CMapGen::GenerateUnhookableLayer()
{
	int Width = g_Config.m_SvGeneratedMapWidth;
	int Height = g_Config.m_SvGeneratedMapHeight;

	CMapItemGroup Item;
	Item.m_Version = CMapItemGroup::CURRENT_VERSION;
	Item.m_ParallaxX = 100;
	Item.m_ParallaxY = 100;
	Item.m_OffsetX = 0;
	Item.m_OffsetY = 0;
	Item.m_StartLayer = m_NumLayers;
	Item.m_NumLayers = 1;
	Item.m_UseClipping = 0;
	Item.m_ClipX = 0;
	Item.m_ClipY = 0;
	Item.m_ClipW = 0;
	Item.m_ClipH = 0;
	StrToInts(Item.m_aName, sizeof(Item.m_aName)/sizeof(int), "Unhookable");

	int ImageUnhookable = AddExternalImage("generic_unhookable", 1024, 1024);
	int RuleUnhookable = LoadRules("generic_unhookable");

	m_pUnhookableTiles = new CTile[Width*Height];
	for(int x = 0;x < Width;x ++)
	{
		for(int y = 0;y < Height;y ++)
		{
			m_pUnhookableTiles[y*Width+x].m_Flags = 0;
			m_pUnhookableTiles[y*Width+x].m_Reserved = 0;
			m_pUnhookableTiles[y*Width+x].m_Skip = 0;
			if(m_pGameTiles[y*Width+x].m_Index == TILE_NOHOOK)
			{
				m_pUnhookableTiles[y*Width+x].m_Index = 1;
			}else 
			{
				m_pUnhookableTiles[y*Width+x].m_Index = 0;
			}
		}
	}
	Proceed(m_pUnhookableTiles, RuleUnhookable);
	AddTile(m_pUnhookableTiles, "Unhookable", ImageUnhookable);
	
	m_DataFile.AddItem(MAPITEMTYPE_GROUP, m_NumGroups++, sizeof(Item), &Item);
}

void CMapGen::GenerateMap()
{

	// save version
	{
		CMapItemVersion Item;
		Item.m_Version = 1;
		m_DataFile.AddItem(MAPITEMTYPE_VERSION, 0, sizeof(Item), &Item);
	}

	// save map info
	{
		CMapItemInfo Item;
		Item.m_Version = 1;
		Item.m_Author = -1;
		Item.m_MapVersion = -1;
		Item.m_Credits = -1;
		Item.m_License = -1;

		m_DataFile.AddItem(MAPITEMTYPE_INFO, 0, sizeof(Item), &Item);
	}

	// Generate background
	GenerateBackground();
	// Generate background tile
	GenerateBackgroundTile();

	// Generate game tile
	GenerateGameLayer();
	
	// Generate doodads tile
	GenerateDoodadsLayer();
	// Generate hookable tile
	GenerateHookableLayer();
	// Generate unhookable tile
	GenerateUnhookableLayer();

}

void CMapGen::Proceed(CTile *pTiles, int ConfigID)
{
	if(ConfigID < 0 || ConfigID >= m_lConfigs.size())
		return;

	CConfiguration *pConf = &m_lConfigs[ConfigID];

	if(!pConf->m_aIndexRules.size())
		return;

	int BaseTile = 1;

	// find base tile if there is one
	for(int i = 0; i < pConf->m_aIndexRules.size(); ++i)
	{
		if(pConf->m_aIndexRules[i].m_BaseTile)
		{
			BaseTile = pConf->m_aIndexRules[i].m_ID;
			break;
		}
	}
	
	int Width = g_Config.m_SvGeneratedMapWidth;
	int Height = g_Config.m_SvGeneratedMapHeight;
	
	int MaxIndex = Width*Height;
	for(int y = 0; y < Height; y++)
		for(int x = 0; x < Width; x++)
		{
			CTile *pTile = &(pTiles[y*Width+x]);
			if(pTile->m_Index == 0)
				continue;

			pTile->m_Index = BaseTile;

			if(y == 0 || y == Height-1 || x == 0 || x == Width-1)
				continue;

			for(int i = 0; i < pConf->m_aIndexRules.size(); ++i)
			{
				if(pConf->m_aIndexRules[i].m_BaseTile)
					continue;

				bool RespectRules = true;
				for(int j = 0; j < pConf->m_aIndexRules[i].m_aRules.size() && RespectRules; ++j)
				{
					CPosRule *pRule = &pConf->m_aIndexRules[i].m_aRules[j];
					int CheckIndex = (y+pRule->m_Y)*Width+(x+pRule->m_X);

					if(CheckIndex < 0 || CheckIndex >= MaxIndex)
						RespectRules = false;
					else
					{
 						if(pRule->m_IndexValue)
						{
							if(pTiles[CheckIndex].m_Index != pRule->m_Value)
								RespectRules = false;
						}
						else
						{
							if(pTiles[CheckIndex].m_Index > 0 && pRule->m_Value == CPosRule::EMPTY)
								RespectRules = false;

							if(pTiles[CheckIndex].m_Index == 0 && pRule->m_Value == CPosRule::FULL)
								RespectRules = false;
						}
					}
				}

				if(RespectRules &&
					(pConf->m_aIndexRules[i].m_RandomValue <= 1 || (int)((float)rand() / ((float)RAND_MAX + 1) * pConf->m_aIndexRules[i].m_RandomValue) == 1))
				{
					pTile->m_Index = pConf->m_aIndexRules[i].m_ID;
					pTile->m_Flags = pConf->m_aIndexRules[i].m_Flag;
				}
			}
		}
}

int CMapGen::LoadRules(const char *pImageName)
{
	char aPath[256];
	str_format(aPath, sizeof(aPath), "mapgen/%s.rules", pImageName);
	IOHANDLE RulesFile = Storage()->OpenFile(aPath, IOFLAG_READ, IStorage::TYPE_ALL);
	if(!RulesFile)
		return -1;

	CLineReader LineReader;
	LineReader.Init(RulesFile);

	CConfiguration *pCurrentConf = 0;
	CIndexRule *pCurrentIndex = 0;

	char aBuf[256];

	// read each line
	while(char *pLine = LineReader.Get())
	{
		// skip blank/empty lines as well as comments
		if(str_length(pLine) > 0 && pLine[0] != '#' && pLine[0] != '\n' && pLine[0] != '\r'
			&& pLine[0] != '\t' && pLine[0] != '\v' && pLine[0] != ' ')
		{
			if(pLine[0]== '[')
			{
				// new configuration, get the name
				pLine++;

				CConfiguration NewConf;
				int ID = m_lConfigs.add(NewConf);
				pCurrentConf = &m_lConfigs[ID];

				str_copy(pCurrentConf->m_aName, pLine, str_length(pLine));
			}
			else
			{
				if(!str_comp_num(pLine, "Index", 5))
				{
					// new index
					int ID = 0;
					char aFlip[128] = "";

					sscanf(pLine, "Index %d %127s", &ID, aFlip);

					CIndexRule NewIndexRule;
					NewIndexRule.m_ID = ID;
					NewIndexRule.m_Flag = 0;
					NewIndexRule.m_RandomValue = 0;
					NewIndexRule.m_BaseTile = false;

					if(str_length(aFlip) > 0)
					{
						if(!str_comp(aFlip, "XFLIP"))
							NewIndexRule.m_Flag = TILEFLAG_VFLIP;
						else if(!str_comp(aFlip, "YFLIP"))
							NewIndexRule.m_Flag = TILEFLAG_HFLIP;
					}

					// add the index rule object and make it current
					int ArrayID = pCurrentConf->m_aIndexRules.add(NewIndexRule);
					pCurrentIndex = &pCurrentConf->m_aIndexRules[ArrayID];
				}
				else if(!str_comp_num(pLine, "BaseTile", 8) && pCurrentIndex)
				{
					pCurrentIndex->m_BaseTile = true;
				}
				else if(!str_comp_num(pLine, "Pos", 3) && pCurrentIndex)
				{
					int x = 0, y = 0;
					char aValue[128];
					int Value = CPosRule::EMPTY;
					bool IndexValue = false;

					sscanf(pLine, "Pos %d %d %127s", &x, &y, aValue);

					if(!str_comp(aValue, "FULL"))
						Value = CPosRule::FULL;
					else if(!str_comp_num(aValue, "INDEX", 5))
					{
						sscanf(pLine, "Pos %*d %*d INDEX %d", &Value);
						IndexValue = true;
					}

					CPosRule NewPosRule = {x, y, Value, IndexValue};
					pCurrentIndex->m_aRules.add(NewPosRule);
				}
				else if(!str_comp_num(pLine, "Random", 6) && pCurrentIndex)
				{
					sscanf(pLine, "Random %d", &pCurrentIndex->m_RandomValue);
				}
			}
		}
	}

	io_close(RulesFile);

	str_format(aBuf, sizeof(aBuf),"loaded %s", aPath);
	Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "mapgen", aBuf);

	return m_lConfigs.size()-1;
}

void CMapGen::AddGameTile(CTile *pTile)
{
	CMapItemLayerTilemap Item;
	Item.m_Version = 3;
	Item.m_Layer.m_Version = 0;
	Item.m_Layer.m_Flags = 0;
	Item.m_Layer.m_Type = LAYERTYPE_TILES;
	Item.m_Color.r = 255;
	Item.m_Color.g = 255;
	Item.m_Color.b = 255;
	Item.m_Color.a = 255;
	Item.m_ColorEnv = -1;
	Item.m_ColorEnvOffset = 0;
	Item.m_Width = g_Config.m_SvGeneratedMapWidth;
	Item.m_Height = g_Config.m_SvGeneratedMapHeight;
	Item.m_Flags = 1;
	Item.m_Image = -1;

	Item.m_Data = m_DataFile.AddData(Item.m_Width*Item.m_Height*sizeof(CTile), pTile);
	StrToInts(Item.m_aName, sizeof(Item.m_aName)/sizeof(int), "Game");
	m_DataFile.AddItem(MAPITEMTYPE_LAYER, m_NumLayers++, sizeof(Item), &Item);
	Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "mapgen", "game tiles generated");
}

void CMapGen::AddTile(CTile *pTile, const char *LayerName, int Image)
{
	CMapItemLayerTilemap Item;
	Item.m_Version = 3;
	Item.m_Layer.m_Version = 0;
	Item.m_Layer.m_Flags = 0;
	Item.m_Layer.m_Type = LAYERTYPE_TILES;
	Item.m_Color.r = 255;
	Item.m_Color.g = 255;
	Item.m_Color.b = 255;
	Item.m_Color.a = 255;
	Item.m_ColorEnv = -1;
	Item.m_ColorEnvOffset = 0;
	Item.m_Width = g_Config.m_SvGeneratedMapWidth;
	Item.m_Height = g_Config.m_SvGeneratedMapHeight;
	Item.m_Flags = 0;
	Item.m_Image = Image;

	Item.m_Data = m_DataFile.AddData(Item.m_Width*Item.m_Height*sizeof(CTile), pTile);
	StrToInts(Item.m_aName, sizeof(Item.m_aName)/sizeof(int), LayerName);
	m_DataFile.AddItem(MAPITEMTYPE_LAYER, m_NumLayers++, sizeof(CMapItemLayerTilemap), &Item);
}

bool CMapGen::CreateMap(const char* pFilename)
{
	char aBuf[512];
	if(!m_DataFile.Open(Storage(), pFilename))
	{
		str_format(aBuf, sizeof(aBuf), "failed to open file '%s'...", pFilename);
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "mapgen", aBuf);
		return false;
	}

	InitState();
	
	GenerateMap();
	
	m_DataFile.Finish();
	
	Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "mapgen", "map generated");
	return true;
}
