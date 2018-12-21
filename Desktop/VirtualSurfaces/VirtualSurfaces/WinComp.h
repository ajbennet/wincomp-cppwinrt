#pragma once

#include "stdafx.h"
#include "TileDrawingManager.h"
#include <winrt/Windows.UI.Composition.Interactions.h>

using namespace winrt;
using namespace winrt::impl;
using namespace Windows::System;
using namespace Windows::UI;
using namespace Windows::UI::Composition;
using namespace Windows::UI::Composition::Interactions;
using namespace Windows::UI::Composition::Desktop;
using namespace Windows::Graphics;
using namespace Windows::Graphics::Display;
using namespace Windows::Graphics::DirectX;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Numerics;


class WinComp: public IInteractionTrackerOwner
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
	VisualInteractionSource m_interactionSource=nullptr;
	InteractionTracker m_tracker = nullptr;
	ExpressionAnimation m_moveSurfaceExpressionAnimation = nullptr;
	ExpressionAnimation m_moveSurfaceUpDownExpressionAnimation = nullptr;
	ExpressionAnimation m_scaleSurfaceUpDownExpressionAnimation = nullptr;

private:

	DesktopWindowTarget CreateDesktopWindowTarget(Compositor const& compositor, HWND window);
	void AddD2DVisual(VisualCollection const& visuals, float x, float y, RECT windowRect);
	void ConfigureInteraction();
	DesktopWindowTarget m_target{ nullptr };
	HWND m_window = nullptr;
	TileDrawingManager m_TileDrawingManager;

};

