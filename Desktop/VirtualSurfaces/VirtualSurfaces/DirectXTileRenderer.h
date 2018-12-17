#pragma once
#include "TileRenderer.h";
#include "stdafx.h"
#include <iostream> 

using namespace Windows::UI;

class DirectXTileRenderer: public TileRenderer
{
public:
	DirectXTileRenderer();
	~DirectXTileRenderer();
	void DrawTile(Rect rect, int tileRow, int tileColumn) override;
	void Trim(Rect trimRect) override;

private:
	int random(int maxValue);
};

