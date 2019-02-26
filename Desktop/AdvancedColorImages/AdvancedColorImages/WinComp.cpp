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

void WinComp::TryRedirectForManipulation(PointerPoint pp)
{
	m_interactionSource.TryRedirectForManipulation(pp);
}

void WinComp::TryUpdatePositionBy(float3 const& amount)
{
	m_tracker.TryUpdatePositionBy(amount);
}

void WinComp::PrepareVisuals()
{
	m_target = CreateDesktopWindowTarget(m_compositor, m_window);
	
	RECT windowRect;
	::GetWindowRect(m_window, &windowRect);

	auto root = m_compositor.CreateSpriteVisual();
	//Create a background with Gray color brush.
	root.Brush(m_compositor.CreateColorBrush({ 0xFF, 0xFE, 0xFE , 0xFE }));

	root.Size({ 0.0f+windowRect.right-windowRect.left, 0.0f + windowRect.bottom-windowRect.top});
	m_target.Root(root);
	
	auto visuals = root.Children();
	AddD2DVisual(visuals, 0.0f, 0.0f, windowRect);
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
	m_contentVisual = compositor.CreateSpriteVisual();
	m_contentVisual.Brush(m_TileDrawingManager.getRenderer()->getSurfaceBrush());

	m_contentVisual.Size({(float)windowRect.right-windowRect.left, (float)windowRect.bottom-windowRect.top});
	m_contentVisual.Offset({ x, y, 0.0f, });

	visuals.InsertAtTop(m_contentVisual);
}

void WinComp::UpdateViewPort(RECT windowRect, boolean changeContentVisual)
{
	Size windowSize;
	windowSize.Height = (windowRect.bottom - windowRect.top)/lastTrackerScale;
	windowSize.Width = (windowRect.right - windowRect.left)/lastTrackerScale;

	m_TileDrawingManager.UpdateViewportSize(windowSize);
	if(changeContentVisual){
		m_contentVisual.Size(windowSize);
	}
}

void WinComp::StartAnimation(CompositionSurfaceBrush brush)
{
	m_animatingPropset = m_compositor.CreatePropertySet();
	m_animatingPropset.InsertScalar(L"xcoord", 1.0f);
	m_animatingPropset.StartAnimation(L"xcoord", m_moveSurfaceExpressionAnimation);

	m_animatingPropset.InsertScalar(L"ycoord", 1.0f);
	m_animatingPropset.StartAnimation(L"ycoord", m_moveSurfaceUpDownExpressionAnimation);

	m_animatingPropset.InsertScalar(L"scale", 1.0f);
	m_animatingPropset.StartAnimation(L"scale", m_scaleSurfaceUpDownExpressionAnimation);

	m_animateMatrix = m_compositor.CreateExpressionAnimation(L"Matrix3x2(props.scale, 0.0, 0.0, props.scale, props.xcoord, props.ycoord)");
	m_animateMatrix.SetReferenceParameter(L"props", m_animatingPropset);

	brush.StartAnimation(L"TransformMatrix", m_animateMatrix);
}

void WinComp::ConfigureInteraction()
{
	m_interactionSource = VisualInteractionSource::Create(m_contentVisual);
	m_interactionSource.PositionXSourceMode(InteractionSourceMode::EnabledWithInertia);
	m_interactionSource.PositionYSourceMode(InteractionSourceMode::EnabledWithInertia);
	m_interactionSource.ScaleSourceMode(InteractionSourceMode::EnabledWithInertia);
	m_interactionSource.ManipulationRedirectionMode(VisualInteractionSourceRedirectionMode::CapableTouchpadAndPointerWheel);

	//IInteractionTrackerOwner itOwner = (auto &&) { make<WinComp>() };
	m_tracker = InteractionTracker::CreateWithOwner(m_compositor, *this);
	m_tracker.InteractionSources().Add(m_interactionSource);
	
	m_moveSurfaceExpressionAnimation = m_compositor.CreateExpressionAnimation(L"-tracker.Position.X");
	m_moveSurfaceExpressionAnimation.SetReferenceParameter(L"tracker", m_tracker);
	
	m_moveSurfaceUpDownExpressionAnimation = m_compositor.CreateExpressionAnimation(L"-tracker.Position.Y");
	m_moveSurfaceUpDownExpressionAnimation.SetReferenceParameter(L"tracker", m_tracker);
	
	m_scaleSurfaceUpDownExpressionAnimation = m_compositor.CreateExpressionAnimation(L"tracker.Scale");
	m_scaleSurfaceUpDownExpressionAnimation.SetReferenceParameter(L"tracker", m_tracker);
	
	m_tracker.MinPosition(float3(0, 0, 0));
	//TODO: use same consts as tilemanager object
	m_tracker.MaxPosition(float3(TileDrawingManager::TILESIZE * 10000, TileDrawingManager::TILESIZE * 10000, 0));

	m_tracker.MinScale(0.1f);
	m_tracker.MaxScale(10.0f);
	
	StartAnimation(m_TileDrawingManager.getRenderer()->getSurfaceBrush());
}

// interactionTrackerowner

void WinComp::CustomAnimationStateEntered(InteractionTracker sender, InteractionTrackerCustomAnimationStateEnteredArgs args)
{
}

void WinComp::IdleStateEntered(InteractionTracker sender, InteractionTrackerIdleStateEnteredArgs args)
{
	if (zooming)
	{
		RECT windowRect;
		::GetWindowRect(m_window, &windowRect);

		//dont update the content visual, because the window size hasnt changed.
		UpdateViewPort(windowRect, false);

	}

	zooming = false;
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
	try
	{
		wstring diags ;

		if (lastTrackerScale == args.Scale())
		{
			diags = m_TileDrawingManager.UpdateVisibleRegion(sender.Position()/lastTrackerScale);
		}
		else
		{
			// Don't run tilemanager during a zoom
			// TODO need custom logic here eg for zoom out
			zooming = true;
		}

		lastTrackerScale = args.Scale();

	}
	catch (...)
	{
	//	Debug.WriteLine(ex.Message);
	}
}







