#pragma once
#include "TileRenderer.h";
#include "stdafx.h"

using namespace Windows::UI;

class DirectXTileRenderer:TileRenderer
{
public:
	DirectXTileRenderer();
	~DirectXTileRenderer();
	void DrawTile(Rect rect, int tileRow, int tileColumn);
	void Trim(Rect trimRect);

private:
	int random(int maxValue);
};

