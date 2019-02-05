#include "stdafx.h"
#include "TileDrawingManager.h"
#include <iostream>

#include <ctime>


TileDrawingManager::TileDrawingManager()
{

}


TileDrawingManager::~TileDrawingManager()
{
}

void TileDrawingManager::setRenderer(DirectXTileRenderer* renderer) {
	currentRenderer = renderer;
};
DirectXTileRenderer* TileDrawingManager::getRenderer()
{
	return currentRenderer;
}

wstring TileDrawingManager::UpdateVisibleRegion(float3 currentPosition)
{
	currentPosition = currentPosition;
	bool stateUpdate = false;

	int requiredTopTileRow = max((int)currentPosition.y / TILESIZE - DrawAheadTileCount, 0);
	int requiredBottomTileRow = (int)(currentPosition.y + viewPortSize.Height) / TILESIZE + DrawAheadTileCount;
	int requiredLeftTileColumn = max((int)currentPosition.x / TILESIZE - DrawAheadTileCount, 0);
	int requiredRightTileColumn = (int)(currentPosition.x + viewPortSize.Width) / TILESIZE + DrawAheadTileCount;

	currentTopLeftTileRow = (int)currentPosition.y / TILESIZE;
	currentTopLeftTileColumn = (int)currentPosition.x / TILESIZE;

	for (int row = requiredTopTileRow; row < drawnTopTileRow; row++)
	{
		for (int column = drawnLeftTileColumn; column <= drawnRightTileColumn; column++)
		{
			DrawTile(row, column);
			stateUpdate = true;
		}
	}

	for (int row = drawnBottomTileRow + 1; row <= requiredBottomTileRow; row++)
	{
		for (int column = drawnLeftTileColumn; column <= drawnRightTileColumn; column++)
		{
			DrawTile(row, column);
			stateUpdate = true;
		}
	}
	drawnTopTileRow = min(requiredTopTileRow, drawnTopTileRow);
	drawnBottomTileRow = max(requiredBottomTileRow, drawnBottomTileRow);


	for (int column = requiredLeftTileColumn; column < drawnLeftTileColumn; column++)
	{
		for (int row = drawnTopTileRow; row <= drawnBottomTileRow; row++)
		{
			DrawTile(row, column);
			stateUpdate = true;
		}
	}


	for (int column = drawnRightTileColumn + 1; column <= requiredRightTileColumn; column++)
	{
		for (int row = drawnTopTileRow; row <= drawnBottomTileRow; row++)
		{
			DrawTile(row, column);
			stateUpdate = true;
		}
	}

	drawnLeftTileColumn = min(requiredLeftTileColumn, drawnLeftTileColumn);
	drawnRightTileColumn = max(requiredRightTileColumn, drawnRightTileColumn);


	//TODO: perf optimization to batch draw tile calls into a single drawingsession scope

	//
	// Consider doing something so that we don't draw tiles above that are simply going to get thrown away 
	// down here - might be as simple as trimming 
	//

	if (stateUpdate)
	{
		Trim(requiredLeftTileColumn, requiredTopTileRow, requiredRightTileColumn, requiredBottomTileRow);
	}

	return L"Left tile:{currentTopLeftTileColumn} Top tile:{currentTopLeftTileRow}";
}

void TileDrawingManager::UpdateViewportSize(Size newSize)
{
	viewPortSize = newSize;
	horizontalVisibleTileCount = (int)ceil(newSize.Width / TILESIZE);
	verticalVisibleTileCount = (int)ceil(newSize.Height / TILESIZE);

	DrawVisibleTiles();
}

Rect TileDrawingManager::GetRectForTile(int row, int column)
{
	int x = column * TILESIZE;
	int y = row * TILESIZE;
	return Rect(x, y, TILESIZE, TILESIZE);
	//TODO: refactor above to use below
}

Rect TileDrawingManager::GetRectForTileRange(int tileStartColumn, int tileStartRow, int numColumns, int numRows)
{
	int x = tileStartColumn * TILESIZE;
	int y = tileStartRow * TILESIZE;
	return Rect(x, y, numColumns * TILESIZE, numRows * TILESIZE);
}

void TileDrawingManager::DrawVisibleTiles()
{
	//currentRenderer->StartDrawingSession();
	//TODO: drawahead applied to left as well
	const clock_t begin_time = std::clock();
	
	for (int row = 0; row < horizontalVisibleTileCount + DrawAheadTileCount; row++)
	{
		for (int column = 0; column <horizontalVisibleTileCount + DrawAheadTileCount; column++)
		{
			DrawTile(row, column);
		}
	}
	
//	currentRenderer->EndDrawingSession();
	drawnRightTileColumn = horizontalVisibleTileCount - 1 + DrawAheadTileCount;
	drawnBottomTileRow = verticalVisibleTileCount - 1 + DrawAheadTileCount;
	// do something

	char msgbuf[1000];
	sprintf_s(msgbuf, "Time Taken %f \n", float(std::clock() - begin_time) / CLOCKS_PER_SEC);
	OutputDebugStringA(msgbuf);

}

void TileDrawingManager::DrawTile(int row, int column)
{
	currentRenderer->DrawTile(GetRectForTile(row, column), row, column); //index's are 0 based
}

void TileDrawingManager::Trim(int leftColumn, int topRow, int rightColumn, int bottomRow)
{
	auto trimRect = GetRectForTileRange(
		leftColumn,
		topRow,
		rightColumn - leftColumn + 1,
		bottomRow - topRow + 1);

	currentRenderer->Trim(trimRect);

	drawnLeftTileColumn = leftColumn;
	drawnRightTileColumn = rightColumn;
	drawnTopTileRow = topRow;
	drawnBottomTileRow = bottomRow;
}
