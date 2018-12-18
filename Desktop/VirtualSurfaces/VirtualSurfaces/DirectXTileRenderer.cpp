#include "stdafx.h"
#include "DirectXTileRenderer.h"
#include "WinComp.h"

using namespace Windows::UI::Composition;


DirectXTileRenderer::DirectXTileRenderer()
{

}


DirectXTileRenderer::~DirectXTileRenderer()
{
}

void DirectXTileRenderer::Initialize() {
	namespace abi = ABI::Windows::UI::Composition;

	com_ptr<ID2D1Factory1> const& factory = CreateFactory();
	com_ptr<ID3D11Device> const& device = CreateDevice();
	com_ptr<IDXGIDevice> const dxdevice = device.as<IDXGIDevice>();

	//TODO: move this out, so renderer is abstracted completely
	m_compositor = WinComp::GetInstance()->m_compositor;

	com_ptr<abi::ICompositorInterop> interopCompositor = m_compositor.as<abi::ICompositorInterop>();
	com_ptr<ID2D1Device> d2device;
	check_hresult(factory->CreateDevice(dxdevice.get(), d2device.put()));
	check_hresult(interopCompositor->CreateGraphicsDevice(d2device.get(), reinterpret_cast<abi::ICompositionGraphicsDevice**>(put_abi(m_graphicsDevice))));

	InitializeTextLayout();
}


CompositionBrush DirectXTileRenderer::getSurfaceBrush()
{
	if (m_CompositionBrush == nullptr) {
		m_CompositionBrush = CreateD2DBrush();
	}
	return m_CompositionBrush;

}

void DirectXTileRenderer::DrawTile(Rect rect, int tileRow, int tileColumn)
{
	Color randomColor = ColorHelper::FromArgb(255, random(256), random(256), random(256));
	// Begin our update of the surface pixels. If this is our first update, we are required
	// to specify the entire surface, which nullptr is shorthand for (but, as it works out,
	// any time we make an update we touch the entire surface, so we always pass nullptr).
	winrt::com_ptr<::ID2D1DeviceContext> d2dDeviceContext;

	POINT offset;
	m_surfaceInterop->BeginDraw(nullptr, __uuidof(ID2D1DeviceContext), (void **)d2dDeviceContext.put(), &offset);
	{
		d2dDeviceContext->Clear(D2D1::ColorF(D2D1::ColorF::Blue, 0.f));
		//Draw the rectangle
		winrt::com_ptr<::ID2D1SolidColorBrush> brush;
		winrt::check_hresult(d2dDeviceContext->CreateSolidColorBrush(
			D2D1::ColorF(D2D1::ColorF::White, 1.0f), brush.put()));
		D2D1_RECT_F source{ rect.X, rect.Y, rect.Width, rect.Height };
		d2dDeviceContext->FillRectangle(source, brush.get());


		//Draw Text
		DrawText(d2dDeviceContext,tileRow, tileColumn);
	}
	m_surfaceInterop->EndDraw();

}

void DirectXTileRenderer::Trim(Rect trimRect)
{
	//drawingSurface.Trim(new RectInt32[]{ new RectInt32 { X = (int)trimRect.X, Y = (int)trimRect.Y, Width = (int)trimRect.Width, Height = (int)trimRect.Height } });
}

int DirectXTileRenderer::random(int maxValue) {

	return rand() % maxValue;

}

// Renders the text into our composition surface
void DirectXTileRenderer::DrawText(com_ptr<ID2D1DeviceContext> d2dDeviceContext,int tileRow, int tileColumn )
{
	winrt::com_ptr<::IDWriteTextLayout> textLayout;

	//Rect windowBounds = { 100,100,100,100 };
	std::wstring text{ std::to_wstring(tileRow) + L"," + std::to_wstring(tileColumn)  };

	winrt::check_hresult(
		m_dWriteFactory->CreateTextLayout(
			text.c_str(),
			(uint32_t)text.size(),
			m_textFormat.get(),
			50,
			50,
			textLayout.put()
		)
	);


	

	// Create a solid color brush for the text. A more sophisticated application might want
	// to cache and reuse a brush across all text elements instead, taking care to recreate
	// it in the event of device removed.
	
	winrt::com_ptr<::ID2D1SolidColorBrush> textBrush;
	winrt::check_hresult(d2dDeviceContext->CreateSolidColorBrush(
			D2D1::ColorF(D2D1::ColorF::Black, 1.0f), textBrush.put()));
	// Draw the line of text at the specified offset, which corresponds to the top-left
	// corner of our drawing surface. Notice we don't call BeginDraw on the D2D device
	// context; this has already been done for us by the composition API.
	d2dDeviceContext->DrawTextLayout(D2D1::Point2F((float)50, (float)50), textLayout.get(),
		textBrush.get());

}

void DirectXTileRenderer::InitializeTextLayout()
{
	
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

}

com_ptr<ID2D1Factory1> DirectXTileRenderer::CreateFactory()
{
	D2D1_FACTORY_OPTIONS options{};
	com_ptr<ID2D1Factory1> factory;

	check_hresult(D2D1CreateFactory(
		D2D1_FACTORY_TYPE_SINGLE_THREADED,
		options,
		factory.put()));

	return factory;
}

HRESULT DirectXTileRenderer::CreateDevice(D3D_DRIVER_TYPE const type, com_ptr<ID3D11Device>& device)
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

com_ptr<ID3D11Device> DirectXTileRenderer::CreateDevice()
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

CompositionDrawingSurface DirectXTileRenderer::CreateVirtualDrawingSurface(SizeInt32 size)
{
	auto graphicsDevice2 = m_graphicsDevice.as<ICompositionGraphicsDevice2>();

	auto surface = graphicsDevice2.CreateVirtualDrawingSurface(
		size,
		DirectXPixelFormat::B8G8R8A8UIntNormalized,
		DirectXAlphaMode::Premultiplied);

	return surface;
}

CompositionBrush DirectXTileRenderer::CreateD2DBrush()
{
	namespace abi = ABI::Windows::UI::Composition;

	SizeInt32 size;
	size.Width = WinComp::TILESIZE * 2;
	size.Height = WinComp::TILESIZE * 2;

	m_surfaceInterop = CreateVirtualDrawingSurface(size).as<abi::ICompositionDrawingSurfaceInterop>();


	ICompositionSurface surface = m_surfaceInterop.as<ICompositionSurface>();

	CompositionSurfaceBrush surfaceBrush = m_compositor.CreateSurfaceBrush(surface);
	surfaceBrush.Stretch(CompositionStretch::None);

	surfaceBrush.HorizontalAlignmentRatio(0);
	surfaceBrush.VerticalAlignmentRatio(0);
	// surfaceBrush.TransformMatrix = System::Numerics::Matrix3x2.CreateTranslation(20.0f, 20.0f);

	//surfaceBrush.Surface = surface;
	CompositionBrush retVal = (CompositionBrush)surfaceBrush;
	return retVal;
}