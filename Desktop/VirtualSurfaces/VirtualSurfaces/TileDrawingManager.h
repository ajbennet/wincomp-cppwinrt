//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THE SOFTWARE IS PROVIDED �AS IS�, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH 
// THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//*********************************************************

#pragma once

#include <iostream>
#include <ctime>
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
	void UpdateVisibleRegion(float3 currentPosition);
	void UpdateViewportSize(Size newSize);
	void setRenderer(DirectXTileRenderer* renderer);
	DirectXTileRenderer* getRenderer();
	void DrawTile(int row, int column);
	const static int TILESIZE = 500;

private:
	
	static const int DRAWAHEAD = 0; //Number of tiles to draw ahead 
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
	Tile GetTileForCoordinates(int row, int column);
	list<Tile> GetTilesForRange(int tileStartColumn, int tileStartRow, int numColumns, int numRows);
	void DrawVisibleTilesbyRange();
	Rect GetRectForTileRange(int tileStartColumn, int tileStartRow, int numColumns, int numRows);
	void DrawVisibleTiles();
	void Trim(int leftColumn, int topRow, int rightColumn, int bottomRow);

};
