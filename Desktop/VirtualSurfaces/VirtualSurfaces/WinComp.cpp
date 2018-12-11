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
	

	com_ptr<ID2D1Factory1> const& factory = CreateFactory();
	com_ptr<ID3D11Device> const& device = CreateDevice();
	com_ptr<IDXGIDevice> const dxdevice = device.as<IDXGIDevice>();

	com_ptr<ID2D1Device> d2device;
	check_hresult(factory->CreateDevice(dxdevice.get(), d2device.put()));

	//check_hresult(d2device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, m_D2DContext.put()));
	Compositor compositor;
	m_compositor = compositor;
	com_ptr<abi::ICompositorInterop> interopCompositor = compositor.as<abi::ICompositorInterop>();
	check_hresult(interopCompositor->CreateGraphicsDevice(d2device.get(), reinterpret_cast<abi::ICompositionGraphicsDevice**>(put_abi(m_graphicsDevice))));

	winrt::check_hresult(
		::DWriteCreateFactory(
			DWRITE_FACTORY_TYPE_SHARED,
			__uuidof(m_dWriteFactory),
			reinterpret_cast<::IUnknown**>(m_dWriteFactory.put())
		)
	);

	winrt::check_hresult(
		m_dWriteFactory->CreateTextFormat(
			L"Segoe UI",
			nullptr,
			DWRITE_FONT_WEIGHT_REGULAR,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			36.f,
			L"en-US",
			m_textFormat.put()
		)
	);

	Rect windowBounds = {100,100,100,100};
	std::wstring text{ L"Hello, World!" };

	winrt::check_hresult(
		m_dWriteFactory->CreateTextLayout(
			text.c_str(),
			(uint32_t)text.size(),
			m_textFormat.get(),
			windowBounds.Width,
			windowBounds.Height,
			m_textLayout.put()
		)
	);
}


void WinComp::PrepareVisuals()
{
	
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

	AddD2DVisual(visuals, 330.0f, 330.0f);
}

void WinComp::AddD2DVisual(VisualCollection const& visuals, float x, float y)
{
	auto compositor = visuals.Compositor();
	auto visual = compositor.CreateSpriteVisual();
	visual.Brush(CreateD2DBrush());

	visual.Size({ 100.0f, 100.0f });
	visual.Offset({ x, y, 0.0f, });

	visuals.InsertAtTop(visual);
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

com_ptr<ID2D1Factory1> WinComp::CreateFactory()
{
	D2D1_FACTORY_OPTIONS options{};
	com_ptr<ID2D1Factory1> factory;

	check_hresult(D2D1CreateFactory(
		D2D1_FACTORY_TYPE_SINGLE_THREADED,
		options,
		factory.put()));

	return factory;
}

HRESULT WinComp::CreateDevice(D3D_DRIVER_TYPE const type, com_ptr<ID3D11Device>& device)
{
	WINRT_ASSERT(!device);

	return D3D11CreateDevice(
		nullptr,
		type,
		nullptr,
		D3D11_CREATE_DEVICE_BGRA_SUPPORT,
		nullptr, 0,
		D3D11_SDK_VERSION,
		device.put(),
		nullptr,
		nullptr);
}

com_ptr<ID3D11Device> WinComp::CreateDevice()
{
	com_ptr<ID3D11Device> device;
	HRESULT hr = CreateDevice(D3D_DRIVER_TYPE_HARDWARE, device);

	if (DXGI_ERROR_UNSUPPORTED == hr)
	{
		hr = CreateDevice(D3D_DRIVER_TYPE_WARP, device);
	}

	check_hresult(hr);
	return device;
}

 com_ptr<ICompositionDrawingSurface> WinComp::CreateSurface(Size size)
{
	com_ptr<ICompositionDrawingSurface> surface;
	check_hresult(m_graphicsDevice->CreateDrawingSurface(
		size,
		DirectXPixelFormat::R8G8B8A8UIntNormalized,
		DirectXAlphaMode::Premultiplied,
		surface.put_void()
	));

	return surface;
}


 CompositionDrawingSurface WinComp::CreateVirtualDrawingSurface(SizeInt32 size)
 {
	 auto graphicsDevice2 = m_graphicsDevice.as<ICompositionGraphicsDevice2>();

	 auto surface = graphicsDevice2.CreateVirtualDrawingSurface(
		 size,
		 DirectXPixelFormat::B8G8R8A8UIntNormalized,
		 DirectXAlphaMode::Premultiplied);

	 return surface;
 }

 CompositionBrush WinComp::CreateD2DBrush( )
 {
	 namespace abi = ABI::Windows::UI::Composition;
	 
	/* Size size;
	 size.Width = 100;
	 size.Height = 100;


	 auto surfaceInterop = CreateSurface(size).as<abi::ICompositionDrawingSurfaceInterop>();*/
	 SizeInt32 size;
	 size.Width = TILESIZE * 2;
	 size.Height = TILESIZE * 2;

	 auto surfaceInterop = CreateVirtualDrawingSurface(size).as<abi::ICompositionDrawingSurfaceInterop>();

	 // Begin our update of the surface pixels. If this is our first update, we are required
	 // to specify the entire surface, which nullptr is shorthand for (but, as it works out,
	 // any time we make an update we touch the entire surface, so we always pass nullptr).
	 winrt::com_ptr<::ID2D1DeviceContext> d2dDeviceContext;

	 POINT offset;
	 surfaceInterop->BeginDraw(nullptr, __uuidof(ID2D1DeviceContext),(void **)d2dDeviceContext.put(), &offset);
	 {
		 D2D1_RECT_F source;
		 source.left = static_cast<float>(offset.x);
		 source.top = static_cast<float>(offset.y);
		 source.right = static_cast<float>(source.left + TILESIZE);
		 source.bottom = static_cast<float>(source.top + TILESIZE);


		DrawText(d2dDeviceContext, offset);


	 }
	 surfaceInterop->EndDraw();

	 ICompositionSurface surface = surfaceInterop.as<ICompositionSurface>();

	 CompositionSurfaceBrush surfaceBrush = m_compositor.CreateSurfaceBrush(surface);

	 
	 //surfaceBrush.Surface = surface;
	 CompositionBrush retVal = (CompositionBrush)surfaceBrush;
	 return retVal;
 }

 // Renders the text into our composition surface
 void WinComp::DrawText(com_ptr<ID2D1DeviceContext> d2dDeviceContext, POINT offset)
 {
	 
		 d2dDeviceContext->Clear(D2D1::ColorF(D2D1::ColorF::Blue, 0.f));

		 // Create a solid color brush for the text. A more sophisticated application might want
		 // to cache and reuse a brush across all text elements instead, taking care to recreate
		 // it in the event of device removed.
		 winrt::com_ptr<::ID2D1SolidColorBrush> brush;
		 winrt::check_hresult(d2dDeviceContext->CreateSolidColorBrush(
			 D2D1::ColorF(D2D1::ColorF::Black, 1.0f), brush.put()));

		 // Draw the line of text at the specified offset, which corresponds to the top-left
		 // corner of our drawing surface. Notice we don't call BeginDraw on the D2D device
		 // context; this has already been done for us by the composition API.
		 d2dDeviceContext->DrawTextLayout(D2D1::Point2F((float)offset.x, (float)offset.y), m_textLayout.get(),
			 brush.get());

 }


