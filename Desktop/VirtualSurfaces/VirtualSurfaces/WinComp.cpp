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
		DQTAT_COM_STA
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
	root.RelativeSizeAdjustment({ 1.0f, 1.0f });
	root.Brush(m_compositor.CreateColorBrush({ 0xFF, 0xEF, 0xE4 , 0xB0 }));

	m_target.Root(root);
	auto visuals = root.Children();
	RECT windowRect;
	::GetWindowRect(m_window, &windowRect);

	AddD2DVisual(visuals, 0, 0, windowRect);
	//m_TileDrawingManager.DrawTile(0, 0);
	//m_TileDrawingManager.DrawTile(0, 1);
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





