#pragma once

#include "stdafx.h"

using namespace winrt;
using namespace Windows::System;
using namespace Windows::UI;
using namespace Windows::UI::Composition;
using namespace Windows::UI::Composition::Desktop;
using namespace Windows::Graphics;
using namespace Windows::Graphics::Display;
using namespace Windows::Graphics::DirectX;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Numerics;

class DirectXTileRenderer
{
public:
	DirectXTileRenderer();
	~DirectXTileRenderer();
	void Initialize();
	void DrawTile(Rect rect, int tileRow, int tileColumn);
	void Trim(Rect trimRect);
	CompositionBrush getSurfaceBrush();

private:
	int random(int maxValue);
	void DrawText(com_ptr<ID2D1DeviceContext> d2dDeviceContext, POINT offset, Rect rect);
	void InitializeTextLayout();
	com_ptr<ID2D1Factory1> CreateFactory();
	HRESULT CreateDevice(D3D_DRIVER_TYPE const type, com_ptr<ID3D11Device>& device);
	com_ptr<ID3D11Device> CreateDevice();
	CompositionBrush CreateD2DBrush();
	CompositionDrawingSurface CreateVirtualDrawingSurface(SizeInt32 size);

	//member variables
	winrt::com_ptr<::IDWriteFactory> m_dWriteFactory;
	winrt::com_ptr<::IDWriteTextFormat> m_textFormat;
	com_ptr<ABI::Windows::UI::Composition::ICompositionDrawingSurfaceInterop> m_surfaceInterop = nullptr;
	com_ptr<ICompositionGraphicsDevice> m_graphicsDevice = nullptr;
	com_ptr<ICompositionGraphicsDevice2>  m_graphicsDevice2 = nullptr;
	com_ptr<ID2D1DeviceContext> m_D2DContext;
	CompositionBrush m_CompositionBrush = nullptr;
	Compositor m_compositor = nullptr;
};

