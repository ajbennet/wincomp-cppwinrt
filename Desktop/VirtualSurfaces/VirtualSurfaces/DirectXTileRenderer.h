//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH 
// THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//*********************************************************

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
	CompositionSurfaceBrush getSurfaceBrush();

private:
	float random(int maxValue);
	void DrawText( int tileRow, int tileColumn, D2D1_RECT_F rect, winrt::com_ptr<::ID2D1DeviceContext> m_d2dDeviceContext,
		winrt::com_ptr<::ID2D1SolidColorBrush> m_textBrush);
	void InitializeTextLayout();
	com_ptr<ID2D1Factory1> CreateFactory();
	HRESULT CreateDevice(D3D_DRIVER_TYPE const type, com_ptr<ID3D11Device>& device);
	com_ptr<ID3D11Device> CreateDevice();
	CompositionSurfaceBrush CreateD2DBrush();
	CompositionDrawingSurface CreateVirtualDrawingSurface(SizeInt32 size);
	bool CheckForDeviceRemoved(HRESULT hr);


	//member variables
	winrt::com_ptr<::IDWriteFactory> m_dWriteFactory;
	winrt::com_ptr<::IDWriteTextFormat> m_textFormat;
	com_ptr<ABI::Windows::UI::Composition::ICompositionDrawingSurfaceInterop> m_surfaceInterop = nullptr;
	com_ptr<ICompositionGraphicsDevice> m_graphicsDevice = nullptr;
	com_ptr<ICompositionGraphicsDevice2>  m_graphicsDevice2 = nullptr;
	CompositionVirtualDrawingSurface m_virtualSurfaceBrush = nullptr;
	CompositionSurfaceBrush m_surfaceBrush = nullptr;
	Compositor m_compositor = nullptr;
	float m_colorCounter = 0.0;
};

