#include "stdafx.h"
#include "DirectXTileRenderer.h"
#include "WinComp.h"
#include <string>
#include <iostream>

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


CompositionSurfaceBrush DirectXTileRenderer::getSurfaceBrush()
{
	if (m_CompositionBrush == nullptr) {
		m_CompositionBrush = CreateD2DBrush();
	}
	return m_CompositionBrush;

}


void DirectXTileRenderer::DrawTile(Rect rect, int tileRow, int tileColumn)
{
	POINT offset;
	RECT updateRect = RECT{ static_cast<LONG>(rect.X),  static_cast<LONG>(rect.Y),  static_cast<LONG>(rect.X + rect.Width-10),  static_cast<LONG>(rect.Y + rect.Height-10) };
	// Begin our update of the surface pixels. If this is our first update, we are required
	// to specify the entire surface, which nullptr is shorthand for (but, as it works out,
	// any time we make an update we touch the entire surface, so we always pass nullptr).
	winrt::com_ptr<::ID2D1DeviceContext> m_d2dDeviceContext;
	winrt::com_ptr<::ID2D1SolidColorBrush> m_textBrush;
	if (CheckForDeviceRemoved(m_surfaceInterop->BeginDraw(&updateRect, __uuidof(ID2D1DeviceContext), (void **)m_d2dDeviceContext.put(), &offset))) {

	m_d2dDeviceContext->Clear(D2D1::ColorF(D2D1::ColorF::Red, 0.f));
	// Create a solid color brush for the text. A more sophisticated application might want
	// to cache and reuse a brush across all text elements instead, taking care to recreate
	// it in the event of device removed.
	winrt::check_hresult(m_d2dDeviceContext->CreateSolidColorBrush(
		D2D1::ColorF(D2D1::ColorF::Black, 1.0f), m_textBrush.put()));


	D2D1::ColorF randomColor(random(256) / 255, random(256) / 255, random(256) / 255, 1.0f);
	//D2D1_RECT_F tileRectangle{ rect.X+offset.x, rect.Y + offset.y, rect.X+offset.x + rect.Width, rect.Y+offset.y + rect.Height };
	D2D1_RECT_F tileRectangle{ offset.x , offset.y , offset.x + rect.Width, offset.y  + rect.Height };


	winrt::com_ptr<::ID2D1SolidColorBrush> tilebrush;
	//Draw the rectangle
	winrt::check_hresult(m_d2dDeviceContext->CreateSolidColorBrush(
		//D2D1::ColorF(D2D1::ColorF::Red, 1.0f), tilebrush.put()));
		randomColor, tilebrush.put()));

	m_d2dDeviceContext->FillRectangle(tileRectangle, tilebrush.get());
	char msgbuf[1000];
	sprintf_s(msgbuf, "Rect %f,%f,%f,%f \n", tileRectangle.left, tileRectangle.top, tileRectangle.right, tileRectangle.bottom);
	OutputDebugStringA(msgbuf);
	memset(msgbuf, 0, 1000);
	sprintf_s(msgbuf, "Tile coordinates : %d,%d \n", tileRow, tileColumn);
	OutputDebugStringA(msgbuf);
	memset(msgbuf, 0, 1000);
	sprintf_s(msgbuf, "Offset %ld,%ld\n", offset.x, offset.y);
	OutputDebugStringA(msgbuf);

	//Draw Text
	DrawText(tileRow, tileColumn, tileRectangle,  m_d2dDeviceContext, m_textBrush);

	m_surfaceInterop->EndDraw();
	}

}


	// We may detect device loss on BeginDraw calls. This helper handles this condition or other
	// errors.
	bool DirectXTileRenderer::CheckForDeviceRemoved(HRESULT hr)
	{
		if (SUCCEEDED(hr))
		{
			// Everything is fine -- go ahead and draw
			return true;
		}
		else if (hr == DXGI_ERROR_DEVICE_REMOVED)
		{
			// We can't draw at this time, but this failure is recoverable. Just skip drawing for
			// now. We will be asked to draw again once the Direct3D device is recreated
			return false;
		}
		else
		{
			// Any other error is unexpected and, therefore, fatal
			//TODO:: FailFast();
		}
	}

void DirectXTileRenderer::Trim(Rect trimRect)
{
	//drawingSurface.Trim(new RectInt32[]{ new RectInt32 { X = (int)trimRect.X, Y = (int)trimRect.Y, Width = (int)trimRect.Width, Height = (int)trimRect.Height } });
}

float DirectXTileRenderer::random(int maxValue) {

	return rand() % maxValue;

}

// Renders the text into our composition surface
void DirectXTileRenderer::DrawText(int tileRow, int tileColumn, D2D1_RECT_F rect, winrt::com_ptr<::ID2D1DeviceContext> m_d2dDeviceContext,
	winrt::com_ptr<::ID2D1SolidColorBrush> m_textBrush)
{
	
	std::wstring text{ std::to_wstring(tileRow) + L"," + std::to_wstring(tileColumn)  };

	winrt::com_ptr<::IDWriteTextLayout> textLayout;
	winrt::check_hresult(
		m_dWriteFactory->CreateTextLayout(
			text.c_str(),
			(uint32_t)text.size(),
			m_textFormat.get(),
			40,
			20,
			textLayout.put()
		)
	);

	// Draw the line of text at the specified offset, which corresponds to the top-left
	// corner of our drawing surface. Notice we don't call BeginDraw on the D2D device
	// context; this has already been done for us by the composition API.
	m_d2dDeviceContext->DrawTextLayout(D2D1::Point2F((float)rect.left+10, (float)rect.top+10), textLayout.get(),
		m_textBrush.get());

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
			20.f,
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

CompositionSurfaceBrush DirectXTileRenderer::CreateD2DBrush()
{
	namespace abi = ABI::Windows::UI::Composition;

	SizeInt32 size;
	size.Width = WinComp::TILESIZE * 10000;
	size.Height = WinComp::TILESIZE * 10000;

	m_surfaceInterop = CreateVirtualDrawingSurface(size).as<abi::ICompositionDrawingSurfaceInterop>();


	ICompositionSurface surface = m_surfaceInterop.as<ICompositionSurface>();

	CompositionSurfaceBrush surfaceBrush = m_compositor.CreateSurfaceBrush(surface);
	surfaceBrush.Stretch(CompositionStretch::None);

	surfaceBrush.HorizontalAlignmentRatio(0);
	surfaceBrush.VerticalAlignmentRatio(0);
	//surfaceBrush.TransformMatrix = System::Numerics::Matrix3x2.CreateTranslation(20.0f, 20.0f);
	surfaceBrush.TransformMatrix(make_float3x2_translation(20.0f, 20.0f));

	return surfaceBrush;
}