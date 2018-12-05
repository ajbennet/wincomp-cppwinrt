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
	m_window = hwnd;
	
}

void WinComp::PrepareVisuals()
{
	Compositor compositor;
	m_target = CreateDesktopWindowTarget(compositor, m_window);
	m_compositor = compositor;
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

void WinComp::AddD2DVisual(VisualCollection const& visuals, float x, float y)
{
	auto compositor = visuals.Compositor();
	auto visual = compositor.CreateSpriteVisual();
	visual.Brush(CreateD2DBrush());
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
	m_graphicsDevice->CreateDrawingSurface(
		size,
		DirectXPixelFormat::R8G8B8A8UIntNormalized,
		DirectXAlphaMode::Premultiplied,
		surface.put_void()
	);

	return surface;
}

 CompositionBrush WinComp::CreateD2DBrush()
 {
	 namespace abi = ABI::Windows::UI::Composition;

	 
	 com_ptr<ID2D1Factory1> const& factory = CreateFactory();
	 com_ptr<ID3D11Device> const& device = CreateDevice();
	 com_ptr<IDXGIDevice> const dxdevice = device.as<IDXGIDevice>();

	 com_ptr<ID2D1Device> d2device;
	 check_hresult(factory->CreateDevice(dxdevice.get(), d2device.put()));

	 com_ptr<ID2D1DeviceContext> target;
	 check_hresult(d2device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, target.put()));


	 Size size;
	 size.Width = 100;
	 size.Height = 100;
	 auto surfaceInterop = CreateSurface(size).as<abi::ICompositionDrawingSurfaceInterop>();



	 com_ptr<ID3D11Texture2D> d3dTexture;

	 POINT offset;
	 surfaceInterop->BeginDraw(nullptr, __uuidof(target),(void **) target.put(), &offset);
	 {
		 D2D1_RECT_F source;
		 source.left = static_cast<float>(offset.x);
		 source.top = static_cast<float>(offset.y);
		 source.right = static_cast<float>(source.left + size.Width);
		 source.bottom = static_cast<float>(source.top + size.Height);


		 //target->DrawBitmap(
			// bitmap.Get(),
			// &source,
			// 1.0,
			// D2D1_INTERPOLATION_MODE_LINEAR,
			// &source
		 //);
	 }
	 surfaceInterop->EndDraw();

	 CompositionSurfaceBrush surfaceBrush = m_compositor.CreateSurfaceBrush();

	 com_ptr<ICompositionSurface> surface = (com_ptr<ICompositionSurface>)surfaceInterop;
	 
	 surfaceBrush.Surface = surface;
	 CompositionBrush retVal = (CompositionBrush)surfaceBrush;
	 return retVal;
 }

