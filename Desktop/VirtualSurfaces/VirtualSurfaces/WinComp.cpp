#include "stdafx.h"
#include "WinComp.h"


WinComp* WinComp::s_instance;

WinComp::WinComp() 
{

}

WinComp* WinComp::GetInstance()
{
	if (s_instance == NULL)
		s_instance = new WinComp();
	return s_instance;
}

WinComp::~WinComp()
{
	delete s_instance;
}

DispatcherQueueController WinComp::EnsureDispatcherQueue()
{
	namespace abi = ABI::Windows::System;

	DispatcherQueueOptions options
	{
		sizeof(DispatcherQueueOptions),
		DQTYPE_THREAD_CURRENT,
		DQTAT_COM_ASTA
	};

	DispatcherQueueController controller{ nullptr };
	check_hresult(CreateDispatcherQueueController(options, reinterpret_cast<abi::IDispatcherQueueController**>(put_abi(controller))));

	return controller;

}

DesktopWindowTarget WinComp::CreateDesktopWindowTarget(Compositor const& compositor, HWND window)
{
	namespace abi = ABI::Windows::UI::Composition::Desktop;

	auto interop = compositor.as<abi::ICompositorDesktopInterop>();
	DesktopWindowTarget target{ nullptr };
	check_hresult(interop->CreateDesktopWindowTarget(window, true, reinterpret_cast<abi::IDesktopWindowTarget**>(put_abi(target))));
	return target;
}

void WinComp::Initialize(HWND hwnd)
{
	namespace abi = ABI::Windows::UI::Composition;

	m_window = hwnd;
	Compositor compositor;
	m_compositor = compositor;
	DirectXTileRenderer* dxRenderer = new DirectXTileRenderer();
	dxRenderer->Initialize();
	m_TileDrawingManager.setRenderer(dxRenderer);

}


void WinComp::PrepareVisuals()
{
	m_target = CreateDesktopWindowTarget(m_compositor, m_window);
	
	auto root = m_compositor.CreateSpriteVisual();
	root.Brush(m_compositor.CreateColorBrush({ 0xFF, 0xEF, 0xE4 , 0xB0 }));

	root.RelativeSizeAdjustment({ 1000, 1000 });
	m_target.Root(root);
	
	m_viewportVisual = m_compositor.CreateSpriteVisual();
	m_viewportVisual.RelativeSizeAdjustment({ 1.0f, 1.0f });
	m_viewportVisual.Brush(m_compositor.CreateColorBrush({ 0xAA, 0xAA, 0xAA, 0xAA }));

	m_contentVisual = m_compositor.CreateContainerVisual();
	m_contentVisual.Size({ 5000, 5000 });

	auto visuals = root.Children();
	visuals.InsertAtTop(m_contentVisual);
	visuals.InsertAtTop(m_viewportVisual);

	visuals = m_contentVisual.Children();
	RECT windowRect;
	::GetWindowRect(m_window, &windowRect);

	AddD2DVisual(visuals, 100.f, 100.0f, windowRect);
//	AddVisual(visuals, 100, 100);
	//m_TileDrawingManager.DrawTile(0, 0);
	//m_TileDrawingManager.DrawTile(0, 1);
}

void WinComp::AddVisual(VisualCollection const& visuals, float x, float y)
{
	auto visual = m_compositor.CreateSpriteVisual();
	static Color colors[] =
	{
		{ 0xDC, 0x5B, 0x9B, 0xD5 },
		{ 0xDC, 0xFF, 0xC0, 0x00 },
		{ 0xDC, 0xED, 0x7D, 0x31 },
		{ 0xDC, 0x70, 0xAD, 0x47 },
	};
	static unsigned last = 0;
	unsigned const next = ++last % _countof(colors);
	visual.Brush(m_compositor.CreateColorBrush(colors[next]));
	visual.Size(
		{
			100.0f,
			100.0f
		});

	visual.Offset(
		{
			x,
			y,
			0.0f,
		});
	visuals.InsertAtTop(visual);
}

void WinComp::AddD2DVisual(VisualCollection const& visuals, float x, float y, RECT windowRect)
{
	auto compositor = visuals.Compositor();
	auto visual = compositor.CreateSpriteVisual();
	visual.Brush(m_TileDrawingManager.getRenderer()->getSurfaceBrush());

	visual.Size({(float)windowRect.right-windowRect.left, (float)windowRect.bottom-windowRect.top});
	visual.Offset({ x, y, 0.0f, });

	visuals.InsertAtTop(visual);
}

void WinComp::DrawVisibleRegion(RECT windowRect) 
{
	Size windowSize;
	windowSize.Height = windowRect.bottom - windowRect.top;
	windowSize.Width = windowRect.right - windowRect.left;

	m_TileDrawingManager.UpdateViewportSize(windowSize);

}

void WinComp::ConfigureInteraction()
{
	m_interactionSource = VisualInteractionSource::Create(m_viewportVisual);
	m_interactionSource.PositionXSourceMode(InteractionSourceMode::EnabledWithInertia);
	m_interactionSource.PositionYSourceMode(InteractionSourceMode::EnabledWithInertia);
	m_interactionSource.ScaleSourceMode(InteractionSourceMode::EnabledWithInertia);
	m_interactionSource.ManipulationRedirectionMode(VisualInteractionSourceRedirectionMode::CapableTouchpadAndPointerWheel);



	m_tracker = InteractionTracker::Create(m_compositor);
	m_tracker.InteractionSources().Add(m_interactionSource);
	
	m_moveSurfaceExpressionAnimation = m_compositor.CreateExpressionAnimation(L"-tracker.Position");
	m_moveSurfaceExpressionAnimation.SetReferenceParameter(L"tracker", m_tracker);
	
	m_moveSurfaceUpDownExpressionAnimation = m_compositor.CreateExpressionAnimation(L"-tracker.Position.Y");
	m_moveSurfaceUpDownExpressionAnimation.SetReferenceParameter(L"tracker", m_tracker);
	
	m_scaleSurfaceUpDownExpressionAnimation = m_compositor.CreateExpressionAnimation(L"tracker.Scale");
	m_scaleSurfaceUpDownExpressionAnimation.SetReferenceParameter(L"tracker", m_tracker);
	
	m_tracker.MinPosition(float3(0, 0, 0));
	//TODO: use same consts as tilemanager object
	m_tracker.MaxPosition(float3(TILESIZE * 10, TILESIZE * 10, 0));

	m_tracker.MinScale(0.01f);
	m_tracker.MaxScale(1.0f);
	m_contentVisual.StartAnimation(L"Offset", m_moveSurfaceExpressionAnimation);
	//m_contentVisual.StartAnimation(L"Offset.Y", m_moveSurfaceUpDownExpressionAnimation);
//	root.StartAnimation(L"Scale", m_scaleSurfaceUpDownExpressionAnimation);
}

// interactionTrackerown

void WinComp::CustomAnimationStateEntered(InteractionTracker sender, InteractionTrackerCustomAnimationStateEnteredArgs args)
{
}

void WinComp::IdleStateEntered(InteractionTracker sender, InteractionTrackerIdleStateEnteredArgs args)
{
	/*if (zooming)
	{
		MessageDialog md = new MessageDialog($"Zoom complete.  Final value:{lastTrackerScale}");
		await md.ShowAsync();
	}

	zooming = false;*/
}

void WinComp::InertiaStateEntered(InteractionTracker sender, InteractionTrackerInertiaStateEnteredArgs args)
{
}

void WinComp::InteractingStateEntered(InteractionTracker sender, InteractionTrackerInteractingStateEnteredArgs args)
{

}

void WinComp::RequestIgnored(InteractionTracker sender, InteractionTrackerRequestIgnoredArgs args)
{
}

void WinComp::ValuesChanged(InteractionTracker sender, InteractionTrackerValuesChangedArgs args)
{
	//try
	//{
	//	string diags = string.Empty;

	//	if (lastTrackerScale == args.Scale)
	//	{
	//		diags = visibleRegionManager.UpdateVisibleRegion(sender.Position);
	//	}
	//	else
	//	{
	//		// Don't run tilemanager during a zoom
	//		// TODO need custom logic here eg for zoom out
	//		zooming = true;
	//	}

	//	lastTrackerScale = args.Scale;

	//	hud.Display = $"X:{sender.Position.X:00000.00} Y:{sender.Position.Y:00000.00} Scale:{sender.Scale:00000.00} " + diags;
	//}
	//catch (Exception ex)
	//{
	//	Debug.WriteLine(ex.Message);
	//}
}






