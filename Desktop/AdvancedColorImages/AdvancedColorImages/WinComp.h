#pragma once

#include "stdafx.h"
#include "TileDrawingManager.h"

using namespace concurrency;
using namespace ::winrt;
using namespace ::winrt::impl;
using namespace winrt::Windows::System;
using namespace winrt::Windows::Storage;
using namespace winrt::Windows::Storage::Streams;
using namespace winrt::Windows::Graphics;
using namespace winrt::Windows::Graphics::Display;
using namespace winrt::Windows::Graphics::DirectX;
using namespace winrt::Windows::UI;
using namespace winrt::Windows::UI::Composition;
using namespace winrt::Windows::UI::Composition::Interactions;
using namespace winrt::Windows::UI::Composition::Desktop;
using namespace winrt::Windows::UI::Input;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Foundation::Numerics;


class WinComp : public implements<WinComp, IInteractionTrackerOwner, no_weak_ref>
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
	void UpdateViewPort(RECT windowRect, boolean changeContentVisual);
	void ConfigureInteraction();
	void LoadDefaultImage();
	void TryRedirectForManipulation(PointerPoint pp);
	void TryUpdatePositionBy(float3 const& amount);

	//interaction tracker owner

	void InertiaStateEntered(InteractionTracker sender, InteractionTrackerInertiaStateEnteredArgs args);
	void InteractingStateEntered(InteractionTracker sender, InteractionTrackerInteractingStateEnteredArgs args);
	void RequestIgnored(InteractionTracker sender, InteractionTrackerRequestIgnoredArgs args);
	void ValuesChanged(InteractionTracker sender, InteractionTrackerValuesChangedArgs args);
	void CustomAnimationStateEntered(InteractionTracker sender, InteractionTrackerCustomAnimationStateEnteredArgs args);
	void IdleStateEntered(InteractionTracker sender, InteractionTrackerIdleStateEnteredArgs args);
	Size getWindowSize();

	const static int TILESIZE = 250;

	Compositor				m_compositor = nullptr;
	VisualInteractionSource m_interactionSource=nullptr;
	SpriteVisual			m_viewportVisual = nullptr;
	SpriteVisual			m_contentVisual = nullptr;
	InteractionTracker		m_tracker = nullptr;

private:

	DesktopWindowTarget CreateDesktopWindowTarget(Compositor const& compositor, HWND window);
	void AddD2DVisual(VisualCollection const& visuals, float x, float y);
	void AddVisual(VisualCollection const& visuals, float x, float y);
	void StartAnimation(CompositionSurfaceBrush brush);
	void LoadImage(_In_ StorageFile const& imageFile);
	void UpdateDefaultRenderOptions();
	void UpdateRenderOptions();
	
	DesktopWindowTarget			m_target{ nullptr };
	HWND						m_window = nullptr;
	TileDrawingManager			m_TileDrawingManager;
	CompositionPropertySet		m_animatingPropset = nullptr;
	ExpressionAnimation			m_animateMatrix = nullptr;
	ImageInfo					m_imageInfo;
	AdvancedColorInfo const&	m_dispInfo{ nullptr };
	bool						m_isImageValid;
	float						m_imageMaxCLL;
	float						m_lastTrackerScale = 1.0f;
	bool						m_zooming;
	ExpressionAnimation			m_moveSurfaceExpressionAnimation = nullptr;
	ExpressionAnimation			m_moveSurfaceUpDownExpressionAnimation = nullptr;
	ExpressionAnimation			m_scaleSurfaceUpDownExpressionAnimation = nullptr;
};

