#include "stdafx.h"
#include "DirectXTileRenderer.h"



DirectXTileRenderer::DirectXTileRenderer()
{
}


DirectXTileRenderer::~DirectXTileRenderer()
{
}

void DirectXTileRenderer::DrawTile(Rect rect, int tileRow, int tileColumn) 
{
	Color randomColor = ColorHelper::FromArgb((byte)255, (byte)random(256), (byte)random(256), (byte)random(256));
	/*using (var drawingSession = CanvasComposition.CreateDrawingSession(drawingSurface, rect))
	{
		drawingSession.Clear(randomColor);

		CanvasTextFormat tf = new CanvasTextFormat(){ FontSize = 72 };
		drawingSession.DrawText($"{tileColumn},{tileRow}", new Vector2(50, 50), Colors.White, tf);
	}*/
}

void DirectXTileRenderer::Trim(Rect trimRect)
{
	//drawingSurface.Trim(new RectInt32[]{ new RectInt32 { X = (int)trimRect.X, Y = (int)trimRect.Y, Width = (int)trimRect.Width, Height = (int)trimRect.Height } });
}

int DirectXTileRenderer::random(int maxValue) {

	return rand() % maxValue;

}