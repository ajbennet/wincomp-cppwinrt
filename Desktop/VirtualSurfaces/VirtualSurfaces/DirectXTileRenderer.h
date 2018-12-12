#pragma once

#include "stdafx.h"

using namespace winrt;
using namespace Windows::UI;
using namespace winrt;
using namespace Windows::Foundation;


class DirectXTileRenderer
{
public:
	DirectXTileRenderer();
	~DirectXTileRenderer();
	void DrawTile(Rect rect, int tileRow, int tileColumn) ;
	void Trim(Rect trimRect) ;

private:
	int random(int maxValue);
};

