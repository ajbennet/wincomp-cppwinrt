#include "stdafx.h"
#include "DirectXTileRenderer.h"
#include "WinComp.h"
#include <string>
#include <iostream>

using namespace Windows::UI::Composition;


static const float sc_MaxZoom = 1.0f; // Restrict max zoom to 1:1 scale.
static const unsigned int sc_MaxBytesPerPixel = 16; // Covers all supported image formats.
static const float sc_nominalRefWhite = 80.0f; // Nominal white nits for sRGB and scRGB.


// 400 bins with gamma of 10 lets us measure luminance to within 10% error for any
// luminance above ~1.5 nits, up to 1 million nits.
static const unsigned int sc_histNumBins = 400;
static const float        sc_histGamma = 0.1f;
static const unsigned int sc_histMaxNits = 1000000;

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
	CreateWicImagingFactory();

	//TODO: move this out, so renderer is abstracted completely
	m_compositor = WinComp::GetInstance()->m_compositor;

	com_ptr<abi::ICompositorInterop> interopCompositor = m_compositor.as<abi::ICompositorInterop>();
	com_ptr<ID2D1Device> d2device;
	check_hresult(factory->CreateDevice(dxdevice.get(), d2device.put()));
	check_hresult(interopCompositor->CreateGraphicsDevice(d2device.get(), reinterpret_cast<abi::ICompositionGraphicsDevice**>(put_abi(m_graphicsDevice))));

	InitializeTextLayout();
}
//
//void DirectXTileRenderer::LoadImage(_In_ StorageFile const& imageFile)
//{
//	create_task(imageFile.OpenAsync(FileAccessMode::Read)
//	).then([=](IRandomAccessStream const& ras) {
//		// If file opening fails, fall through to error handler at the end of task chain.
//
//		com_ptr<IStream> iStream;
//		check_hresult(
//			CreateStreamOverRandomAccessStream(winrt::get_unknown(ras), __uuidof(iStream), iStream.put_void())
//		);
//
//		return LoadImageFromWic(iStream.get());
//		});
//}
//

CompositionSurfaceBrush DirectXTileRenderer::getSurfaceBrush()
{
	if (m_surfaceBrush == nullptr) {
		m_surfaceBrush = CreateD2DBrush();
	}
	return m_surfaceBrush;

}

// White level scale is used to multiply the color values in the image; allows the user to
// adjust the brightness of the image on an HDR display.
void DirectXTileRenderer::SetRenderOptions(
	RenderEffectKind effect,
	float brightnessAdjustment,
	AdvancedColorInfo const& acInfo
)
{
	m_dispInfo = acInfo;
	m_renderEffectKind = effect;
	m_brightnessAdjust = brightnessAdjustment;

	auto sdrWhite = m_dispInfo ?  m_dispInfo.SdrWhiteLevelInNits(): sc_nominalRefWhite;

	UpdateWhiteLevelScale(m_brightnessAdjust, sdrWhite);

	// Adjust the Direct2D effect graph based on RenderEffectKind.
	// Some RenderEffectKind values require us to apply brightness adjustment
	// after the effect as their numerical output is affected by any luminance boost.
	switch (m_renderEffectKind)
	{
	
		// Effect graph: ImageSource > ColorManagement > WhiteScale
	case RenderEffectKind::None:
		m_whiteScaleEffect.as(m_finalOutput);
		m_whiteScaleEffect->SetInputEffect(0, m_colorManagementEffect.get());
		break;

	}

	Draw(Rect(0, 0, 800, 800));
}

// When connected to an HDR display, the OS renders SDR content (e.g. 8888 UNORM) at
// a user configurable white level; this typically is around 200-300 nits. It is the responsibility
// of an advanced color app (e.g. FP16 scRGB) to emulate the OS-implemented SDR white level adjustment,
// BUT only for non-HDR content (SDR or WCG).
void DirectXTileRenderer::UpdateWhiteLevelScale(float brightnessAdjustment, float sdrWhiteLevel)
{
	float scale = 1.0f;

	switch (m_imageInfo.imageKind)
	{
	case AdvancedColorKind::HighDynamicRange:
		// HDR content should not be compensated by the SdrWhiteLevel parameter.
		scale = 1.0f;
		break;

	case AdvancedColorKind::StandardDynamicRange:
	case AdvancedColorKind::WideColorGamut:
	default:
		scale = sdrWhiteLevel / sc_nominalRefWhite;
		break;
	}

	// The user may want to manually adjust brightness specifically for this image, on top of any
	// white level adjustment for SDR/WCG content. Brightness adjustment using a linear gamma scale
	// is mainly useful for HDR displays, but can be useful for HDR content tonemapped to an SDR/WCG display.
	scale *= brightnessAdjustment;

	// SDR white level scaling is performing by multiplying RGB color values in linear gamma.
	// We implement this with a Direct2D matrix effect.
	D2D1_MATRIX_5X4_F matrix = D2D1::Matrix5x4F(
		scale, 0, 0, 0,  // [R] Multiply each color channel
		0, scale, 0, 0,  // [G] by the scale factor in 
		0, 0, scale, 0,  // [B] linear gamma space.
		0, 0, 0, 1,  // [A] Preserve alpha values.
		0, 0, 0, 0); //     No offset.

	check_hresult(m_whiteScaleEffect->SetValue(D2D1_COLORMATRIX_PROP_COLOR_MATRIX, matrix));
}

// Reads the provided data stream and decodes an image from it using WIC. These resources are device-
// independent.
ImageInfo DirectXTileRenderer::LoadImageFromWic(_In_ IStream* imageStream)
{
	
	// Decode the image using WIC.
	com_ptr<IWICBitmapDecoder> decoder;
	check_hresult(
		m_wicFactory->CreateDecoderFromStream(
			imageStream,
			nullptr,
			WICDecodeMetadataCacheOnDemand,
			decoder.put()
		));

	com_ptr<IWICBitmapFrameDecode> frame;
	check_hresult(
		decoder->GetFrame(0, frame.put())
	);

	return LoadImageCommon(frame.get());
}


// Reads the provided File and decodes an image from it using WIC. These resources are device-
// independent.
ImageInfo DirectXTileRenderer::LoadImageFromWic(LPCWSTR szFileName)
{

		// Create a decoder
		IWICBitmapDecoder *pDecoder = nullptr;

		// Decode the image using WIC.
		com_ptr<IWICBitmapDecoder> decoder;
		check_hresult(
			m_wicFactory->CreateDecoderFromFilename(
				szFileName,                      // Image to be decoded
				nullptr,                         // Do not prefer a particular vendor
				GENERIC_READ,                    // Desired read access to the file
				WICDecodeMetadataCacheOnDemand,  // Cache metadata when needed
				decoder.put()					 // Pointer to the decoder
			));


		// Retrieve the first frame of the image from the decoder
		com_ptr<IWICBitmapFrameDecode> frame;
		check_hresult(
			decoder->GetFrame(0, frame.put())
		);

		return LoadImageCommon(frame.get());

}



// After initial decode, obtain image information and do common setup.
// Populates all members of ImageInfo.
ImageInfo DirectXTileRenderer::LoadImageCommon(_In_ IWICBitmapSource* source)
{
	m_imageInfo = {};

	// Attempt to read the embedded color profile from the image; only valid for WIC images.
	com_ptr<IWICBitmapFrameDecode> frame;
	HRESULT hr = (reinterpret_cast<::IUnknown*>(source)->QueryInterface(winrt::guid_of<IWICBitmapFrameDecode>(),
		reinterpret_cast<void**>(winrt::put_abi(frame))));
	if (hr>=0)
	{
		check_hresult(
			m_wicFactory->CreateColorContext(m_wicColorContext.put())
		);

		check_hresult(
			frame->GetColorContexts(
				1,
				m_wicColorContext.put(),
				&m_imageInfo.numProfiles
			)
		);
	}

	// Check whether the image data is natively stored in a floating-point format, and
	// decode to the appropriate WIC pixel format.

	WICPixelFormatGUID pixelFormat;
	check_hresult(
		source->GetPixelFormat(&pixelFormat)
	);

	com_ptr<IWICComponentInfo> componentInfo;
	check_hresult(
		m_wicFactory->CreateComponentInfo(
			pixelFormat,
			componentInfo.put()
		)
	);

	com_ptr<IWICPixelFormatInfo2> pixelFormatInfo = componentInfo.as<IWICPixelFormatInfo2>();
	

	WICPixelFormatNumericRepresentation formatNumber;
	check_hresult(
		pixelFormatInfo->GetNumericRepresentation(&formatNumber)
	);

	check_hresult(pixelFormatInfo->GetBitsPerPixel(&m_imageInfo.bitsPerPixel));

	// Calculate the bits per channel (bit depth) using GetChannelMask.
	// This accounts for nonstandard color channel packing and padding, e.g. 32bppRGB.
	unsigned char channelMaskBytes[sc_MaxBytesPerPixel];
	ZeroMemory(channelMaskBytes, ARRAYSIZE(channelMaskBytes));
	unsigned int maskSize;

	check_hresult(
		pixelFormatInfo->GetChannelMask(
			0,  // Read the first color channel.
			ARRAYSIZE(channelMaskBytes),
			channelMaskBytes,
			&maskSize)
	);

	// Count up the number of bits set in the mask for the first color channel.
	for (unsigned int i = 0; i < maskSize * 8; i++)
	{
		unsigned int byte = i / 8;
		unsigned int bit = i % 8;
		if ((channelMaskBytes[byte] & (1 << bit)) != 0)
		{
			m_imageInfo.bitsPerChannel += 1;
		}
	}

	m_imageInfo.isFloat = (WICPixelFormatNumericRepresentationFloat == formatNumber) ? true : false;

	// When decoding, preserve the numeric representation (float vs. non-float)
	// of the native image data. This avoids WIC performing an implicit gamma conversion
	// which occurs when converting between a fixed-point/integer pixel format (sRGB gamma)
	// and a float-point pixel format (linear gamma). Gamma adjustment, if specified by
	// the ICC profile, will be performed by the Direct2D color management effect.

	WICPixelFormatGUID fmt = {};
	if (m_imageInfo.isFloat)
	{
		fmt = GUID_WICPixelFormat64bppPRGBAHalf; // Equivalent to DXGI_FORMAT_R16G16B16A16_FLOAT.
	}
	else
	{
		fmt = GUID_WICPixelFormat64bppPRGBA; // Equivalent to DXGI_FORMAT_R16G16B16A16_UNORM.
											 // Many SDR images (e.g. JPEG) use <=32bpp, so it
											 // is possible to further optimize this for memory usage.
	}

	check_hresult(
		m_wicFactory->CreateFormatConverter(m_formatConvert.put())
	);

	check_hresult(
		m_formatConvert->Initialize(
			source,
			fmt,
			WICBitmapDitherTypeNone,
			nullptr,
			0.0f,
			WICBitmapPaletteTypeCustom
		)
	);

	UINT width;
	UINT height;
	check_hresult(
		m_formatConvert->GetSize(&width, &height)
	);

	m_imageInfo.size = Size(static_cast<float>(width), static_cast<float>(height));

	PopulateImageInfoACKind(&m_imageInfo);

	return m_imageInfo;
}




// Simplified heuristic to determine what advanced color kind the image is.
// Requires that all fields other than imageKind are populated.
void DirectXTileRenderer::PopulateImageInfoACKind(_Inout_ ImageInfo* info)
{
	if (info->bitsPerPixel == 0 ||
		info->bitsPerChannel == 0 ||
		info->size.Width == 0 ||
		info->size.Height == 0)
	{
		check_hresult(E_INVALIDARG);
	}

	info->imageKind = AdvancedColorKind::StandardDynamicRange;

	// Bit depth > 8bpc or color gamut > sRGB signifies a WCG image.
	// The presence of a color profile is used as an approximation for wide gamut.
	if (info->bitsPerChannel > 8 || info->numProfiles >= 1)
	{
		info->imageKind = AdvancedColorKind::WideColorGamut;
	}

	// This application currently only natively supports HDR images with floating point.
	// An image encoded using the HDR10 colorspace is also HDR, but this
	// is not automatically detected by the application.
	if (info->isFloat == true)
	{
		info->imageKind = AdvancedColorKind::HighDynamicRange;
	}
}

void DirectXTileRenderer::Draw(Rect rect)
{
	POINT offset;
	RECT updateRect = RECT{ static_cast<LONG>(rect.X),  static_cast<LONG>(rect.Y),  static_cast<LONG>(rect.X + rect.Width - 5),  static_cast<LONG>(rect.Y + rect.Height - 5) };
	// Begin our update of the surface pixels. If this is our first update, we are required
	// to specify the entire surface, which nullptr is shorthand for (but, as it works out,
	// any time we make an update we touch the entire surface, so we always pass nullptr).
	winrt::com_ptr<::ID2D1DeviceContext> d2dDeviceContext;
	if (CheckForDeviceRemoved(m_surfaceInterop->BeginDraw(&updateRect, __uuidof(ID2D1DeviceContext), (void **)d2dDeviceContext.put(), &offset))) {

		d2dDeviceContext->Clear(D2D1::ColorF(D2D1::ColorF::Black, 0.f));
		
		if (m_scaledImage)
		{
			d2dDeviceContext->DrawImage(m_finalOutput.get(), m_imageOffset);

			//EmitHdrMetadata();
		}

		m_surfaceInterop->EndDraw();
	}

}




void DirectXTileRenderer::DrawTile(Rect rect, int tileRow, int tileColumn)
{
	POINT offset;
	RECT updateRect = RECT{ static_cast<LONG>(rect.X),  static_cast<LONG>(rect.Y),  static_cast<LONG>(rect.X + rect.Width-5),  static_cast<LONG>(rect.Y + rect.Height-5)};
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
		D2D1::ColorF(D2D1::ColorF::DimGray, 1.0f), m_textBrush.put()));

	//Generating colors to distinguish each tile.
	m_colorCounter = (int)(m_colorCounter+8) % 192 + 8.0f;
	D2D1::ColorF randomColor( m_colorCounter/256, 1.0f, 0.0f, 0.5f);
	D2D1_RECT_F tileRectangle{ offset.x , offset.y , offset.x + rect.Width, offset.y  + rect.Height };

	winrt::com_ptr<::ID2D1SolidColorBrush> tilebrush;
	//Draw the rectangle
	winrt::check_hresult(m_d2dDeviceContext->CreateSolidColorBrush(
		randomColor, tilebrush.put()));

	m_d2dDeviceContext->FillRectangle(tileRectangle, tilebrush.get());
	/*char msgbuf[1000];
	sprintf_s(msgbuf, "Rect %f,%f,%f,%f \n", tileRectangle.left, tileRectangle.top, tileRectangle.right, tileRectangle.bottom);
	OutputDebugStringA(msgbuf);
	memset(msgbuf, 0, 1000);
	sprintf_s(msgbuf, "Tile coordinates : %d,%d \n", tileRow, tileColumn);
	OutputDebugStringA(msgbuf);
	memset(msgbuf, 0, 1000);
	sprintf_s(msgbuf, "Offset %ld,%ld\n", offset.x, offset.y);
	OutputDebugStringA(msgbuf);
	*/
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
	RectInt32 trimRects[1];
	trimRects[0] = RectInt32{ (int)trimRect.X, (int)trimRect.Y, (int)trimRect.Width, (int)trimRect.Height };
	
	m_virtualSurfaceBrush.Trim(trimRects);
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
			60,
			30,
			textLayout.put()
		)
	);

	// Draw the line of text at the specified offset, which corresponds to the top-left
	// corner of our drawing surface. Notice we don't call BeginDraw on the D2D device
	// context; this has already been done for us by the composition API.
	//position the text in the center as much as possible.
	m_d2dDeviceContext->DrawTextLayout(D2D1::Point2F((float)rect.left + (TileDrawingManager::TILESIZE / 2 - 30), (float)rect.top+(TileDrawingManager::TILESIZE/2-30)), textLayout.get(),
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
			30.f,
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

	m_virtualSurfaceBrush = graphicsDevice2.CreateVirtualDrawingSurface(
		size,
		DirectXPixelFormat::B8G8R8A8UIntNormalized,
		DirectXAlphaMode::Premultiplied);

	return m_virtualSurfaceBrush;
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
	surfaceBrush.TransformMatrix(make_float3x2_translation(20.0f, 20.0f));

	return surfaceBrush;
}

void DirectXTileRenderer::CreateD2DContext(com_ptr<ID3D11Device> d3dDevice, com_ptr<ID2D1Factory1> d2dFactory)
{
	// Create the Direct2D device object and a corresponding context.
	com_ptr<IDXGIDevice3> dxgiDevice;
	dxgiDevice = d3dDevice.as<IDXGIDevice3>();

	com_ptr<ID2D1Device>            d2dDeviceTemp;
	check_hresult(
		d2dFactory->CreateDevice(dxgiDevice.get(), d2dDeviceTemp.put())
	);
	com_ptr<ID2D1Device5>            d2dDevice;
	d2dDevice = d2dDeviceTemp.as<ID2D1Device5>();

	check_hresult(
		d2dDevice->CreateDeviceContext(
			D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
			m_d2dContext.put()
		)
	);
}

void DirectXTileRenderer::CreateWicImagingFactory()
{
	check_hresult(
		CoCreateInstance(
			CLSID_WICImagingFactory2,
			nullptr,
			CLSCTX_INPROC_SERVER,
			__uuidof(m_wicFactory),
			m_wicFactory.put_void())
	);
}

// Overrides any pan/zoom state set by the user to fit image to the window size.
// Returns the computed MaxCLL of the image in nits.
float DirectXTileRenderer::FitImageToWindow(Size panelSize)
{
	if (m_imageSource)
	{
		// Set image to be letterboxed in the window, up to the max allowed scale factor.
		float letterboxZoom = min(
			panelSize.Width / m_imageInfo.size.Width,
			panelSize.Height / m_imageInfo.size.Height);

		m_zoom = min(sc_MaxZoom, letterboxZoom);

		// Center the image.
		m_imageOffset = D2D1::Point2F(
			(panelSize.Width - (m_imageInfo.size.Width * m_zoom)) / 2.0f,
			(panelSize.Height - (m_imageInfo.size.Height * m_zoom)) / 2.0f
		);

		//UpdateImageTransformState();

		// HDR metadata is supposed to be independent of any rendering options, but
		// we can't compute it until the full effect graph is hooked up, which is here.
		//ComputeHdrMetadata();
	}

	return m_maxCLL;
}