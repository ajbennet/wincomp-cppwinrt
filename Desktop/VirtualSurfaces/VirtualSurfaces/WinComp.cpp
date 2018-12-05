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

	Windows::System::DispatcherQueueController controller{ nullptr };
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
	m_window = hwnd;
	
}

void WinComp::PrepareVisuals()
{
	Compositor m_compositor;
	m_target = CreateDesktopWindowTarget(m_compositor, m_window);
	auto root = m_compositor.CreateSpriteVisual();
	root.RelativeSizeAdjustment({ 1.0f, 1.0f });
	root.Brush(m_compositor.CreateColorBrush({ 0xFF, 0xEF, 0xE4 , 0xB0 }));
	m_target.Root(root);
	auto visuals = root.Children();

	AddVisual(visuals, 100.0f, 100.0f);
	AddVisual(visuals, 220.0f, 100.0f);
	AddVisual(visuals, 100.0f, 220.0f);
	AddVisual(visuals, 220.0f, 220.0f);
}

void WinComp::AddVisual(VisualCollection const& visuals, float x, float y)
{
	auto compositor = visuals.Compositor();
	auto visual = compositor.CreateSpriteVisual();

	static Color colors[] =
	{
		{ 0xDC, 0x5B, 0x9B, 0xD5 },
	{ 0xDC, 0xFF, 0xC0, 0x00 },
	{ 0xDC, 0xED, 0x7D, 0x31 },
	{ 0xDC, 0x70, 0xAD, 0x47 },
	};

	static unsigned last = 0;
	unsigned const next = ++last % _countof(colors);
	visual.Brush(compositor.CreateColorBrush(colors[next]));
	visual.Size({ 100.0f, 100.0f });
	visual.Offset({ x, y, 0.0f, });

	visuals.InsertAtTop(visual);
}
