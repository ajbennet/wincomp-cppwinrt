#pragma once

#include "stdafx.h"

using namespace winrt;
using namespace Windows::System;
using namespace Windows::UI;
using namespace Windows::UI::Composition;
using namespace Windows::UI::Composition::Desktop;
using namespace Windows::Graphics::Display;
using namespace Windows::Graphics::DirectX;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Numerics;



class WinComp
{
#pragma region Singleton Pattern
public:
	~WinComp();
	static WinComp* GetInstance();
private:
	WinComp();
	static WinComp* s_instance;
#pragma endregion

public:
	void Initialize(HWND hwnd);
	void PrepareVisuals();
	DispatcherQueueController EnsureDispatcherQueue();
	

private:

	DesktopWindowTarget CreateDesktopWindowTarget(Compositor const& compositor, HWND window);
	void AddVisual(VisualCollection const& visuals, float x, float y);
	com_ptr<ID2D1Factory1> CreateFactory();
	HRESULT CreateDevice(D3D_DRIVER_TYPE const type, com_ptr<ID3D11Device>& device);
	com_ptr<ID3D11Device> CreateDevice();
	com_ptr<ICompositionDrawingSurface> CreateSurface(Size size);
	
	void AddD2DVisual(VisualCollection const& visuals, float x, float y);
	void DrawText(com_ptr<ID2D1DeviceContext>, POINT offset);
	CompositionBrush CreateD2DBrush();

	DesktopWindowTarget m_target{ nullptr };
	HWND m_window = nullptr;
	com_ptr<Windows::UI::Composition::ICompositionGraphicsDevice> m_graphicsDevice = nullptr;
	com_ptr<ID2D1DeviceContext> m_D2DContext;


	// The text to draw.
	winrt::com_ptr<::IDWriteFactory> m_dWriteFactory;
	winrt::com_ptr<::IDWriteTextFormat> m_textFormat;
	winrt::com_ptr<::IDWriteTextLayout> m_textLayout;
	

	Compositor m_compositor = nullptr;

};

