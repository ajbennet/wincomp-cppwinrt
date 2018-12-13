#pragma once
#include "stdafx.h";
#include "DirectXTileRenderer.h"

using namespace std;
using namespace winrt;
using namespace Windows::Foundation::Numerics;
using namespace Windows::Foundation;

class TileDrawingManager
{
public:
	TileDrawingManager();
	~TileDrawingManager();
	wstring UpdateVisibleRegion(float3 currentPosition);
	void UpdateViewportSize(Size newSize);
	void setRenderer(DirectXTileRenderer* renderer);
	DirectXTileRenderer* getRenderer();
	void DrawTile(int row, int column);

private:
	const int TILESIZE = 250;
	const int DRAWAHEAD = 0; //Number of tiles to draw ahead //Note: drawahead doesn't currently work
	int currentTopLeftTileRow = 0;
	int currentTopLeftTileColumn = 0;
	int drawnTopTileRow = 0;
	int drawnBottomTileRow = 0;
	int drawnLeftTileColumn = 0;
	int drawnRightTileColumn = 0;
	Size viewPortSize;
	int horizontalVisibleTileCount;
	int verticalVisibleTileCount;
	float3 currentPosition;
	DirectXTileRenderer* currentRenderer;
	int DrawAheadTileCount;
	Rect GetRectForTile(int row, int column);
	Rect GetRectForTileRange(int tileStartColumn, int tileStartRow, int numColumns, int numRows);
	void DrawVisibleTiles();
	void Trim(int leftColumn, int topRow, int rightColumn, int bottomRow);

};

