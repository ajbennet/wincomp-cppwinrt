#pragma once

#include "stdafx.h"
#include "TileDrawingManager.h"

using namespace winrt;
using namespace winrt::impl;
using namespace Windows::System;
using namespace Windows::UI;
using namespace Windows::UI::Composition;
using namespace Windows::UI::Composition::Desktop;
using namespace Windows::Graphics;
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
	void DrawVisibleRegion(RECT windowRect);
	Compositor m_compositor = nullptr;
	const static int TILESIZE = 100;

private:

	DesktopWindowTarget CreateDesktopWindowTarget(Compositor const& compositor, HWND window);
	void AddD2DVisual(VisualCollection const& visuals, float x, float y);

	DesktopWindowTarget m_target{ nullptr };
	HWND m_window = nullptr;
	TileDrawingManager m_TileDrawingManager;

};

