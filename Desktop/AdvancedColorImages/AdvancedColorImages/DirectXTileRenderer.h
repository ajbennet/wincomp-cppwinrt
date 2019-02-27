#pragma once

#include "stdafx.h"
#include "RenderOptions.h"

using namespace winrt;
using namespace Windows::System;
using namespace Windows::UI;
using namespace Windows::UI::Composition;
using namespace Windows::UI::Composition::Desktop;
using namespace Windows::Graphics;
using namespace Windows::Graphics::Display;
using namespace Windows::Graphics::DirectX;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Numerics;

struct ImageInfo
{
	unsigned int                                    bitsPerPixel;
	unsigned int                                    bitsPerChannel;
	bool                                            isFloat;
	Windows::Foundation::Size                       size;
	unsigned int                                    numProfiles;
	Windows::Graphics::Display::AdvancedColorKind   imageKind;
};

/// <summary>
/// Supported render effects which are inserted into the render pipeline.
/// Includes HDR tonemappers and useful visual tools.
/// Each render effect is implemented as a custom Direct2D effect.
/// </summary>
enum class RenderEffectKind
{
	//ReinhardTonemap,
	//FilmicTonemap,
	None,
	//SdrOverlay,
	//LuminanceHeatmap
};

class DirectXTileRenderer
{
public:
	DirectXTileRenderer();
	~DirectXTileRenderer();
	void Initialize();
	void DrawTile(Rect rect, int tileRow, int tileColumn);
	void Trim(Rect trimRect);
	CompositionSurfaceBrush getSurfaceBrush();
	void SetRenderOptions(RenderEffectKind effect, float brightnessAdjustment, AdvancedColorInfo const& acInfo);
	float FitImageToWindow(Size panelSize);
	ImageInfo LoadImageFromWic(_In_ IStream* imageStream);


private:
	float random(int maxValue);
	void DrawText( int tileRow, int tileColumn, D2D1_RECT_F rect, winrt::com_ptr<::ID2D1DeviceContext> m_d2dDeviceContext,
		winrt::com_ptr<::ID2D1SolidColorBrush> m_textBrush);
	void InitializeTextLayout();
	com_ptr<ID2D1Factory1> CreateFactory();
	HRESULT CreateDevice(D3D_DRIVER_TYPE const type, com_ptr<ID3D11Device>& device);
	com_ptr<ID3D11Device> CreateDevice();
	void CreateD2DContext(com_ptr<ID3D11Device> d3dDevice, com_ptr<ID2D1Factory1> d2dFactory);
	CompositionSurfaceBrush CreateD2DBrush();
	CompositionDrawingSurface CreateVirtualDrawingSurface(SizeInt32 size);
	bool CheckForDeviceRemoved(HRESULT hr);

	void UpdateWhiteLevelScale(float brightnessAdjustment, float sdrWhiteLevel);
	ImageInfo LoadImageCommon(_In_ IWICBitmapSource* source);
	void PopulateImageInfoACKind(_Inout_ ImageInfo* info);
	void Draw(Rect rect);


	//member variables
	com_ptr<::IDWriteFactory>				m_dWriteFactory;
	com_ptr<ID2D1DeviceContext5>			m_d2dContext;
	com_ptr<::IDWriteTextFormat>			m_textFormat;
	com_ptr<ICompositionGraphicsDevice>		m_graphicsDevice = nullptr;
	com_ptr<ICompositionGraphicsDevice2>	m_graphicsDevice2 = nullptr;
	CompositionVirtualDrawingSurface		m_virtualSurfaceBrush = nullptr;
	CompositionSurfaceBrush					m_surfaceBrush = nullptr;
	Compositor								m_compositor = nullptr;
	float									m_colorCounter = 0.0;
	com_ptr<ABI::Windows::UI::Composition::ICompositionDrawingSurfaceInterop> m_surfaceInterop = nullptr;


	//Advanced color 

	
	// WIC and Direct2D resources.
	com_ptr<IWICFormatConverter>             m_formatConvert;
	com_ptr<IWICColorContext>                m_wicColorContext;
	com_ptr<ID2D1ImageSourceFromWic>         m_imageSource;
	com_ptr<ID2D1TransformedImageSource>     m_scaledImage;
	com_ptr<ID2D1Effect>                     m_colorManagementEffect;
	com_ptr<ID2D1Effect>                     m_whiteScaleEffect;
	com_ptr<ID2D1Effect>                     m_reinhardEffect;
	com_ptr<ID2D1Effect>                     m_filmicEffect;
	com_ptr<ID2D1Effect>                     m_sdrOverlayEffect;
	com_ptr<ID2D1Effect>                     m_heatmapEffect;
	com_ptr<ID2D1Effect>                     m_histogramPrescale;
	com_ptr<ID2D1Effect>                     m_histogramEffect;
	com_ptr<ID2D1Effect>                     m_finalOutput;
	com_ptr<IWICImagingFactory2>			 m_wicFactory;

	// Other renderer members.
	RenderEffectKind                        m_renderEffectKind;
	float                                   m_zoom;
	float                                   m_minZoom;
	D2D1_POINT_2F                           m_imageOffset;
	D2D1_POINT_2F                           m_pointerPos;
	float                                   m_maxCLL; // In nits.
	float                                   m_brightnessAdjust;
	AdvancedColorInfo						m_dispInfo{nullptr};
	ImageInfo                               m_imageInfo;
	bool                                    m_isComputeSupported;
};

