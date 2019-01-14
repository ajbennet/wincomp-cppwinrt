// App.cpp
//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH 
// THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//*********************************************************

#include "pch.h"
#include "CustomTextRenderer.h"

#include <Windows.Graphics.DirectX.Direct3D11.interop.h>
#include <Windows.ui.composition.interop.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.DirectX.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>
#include <winrt/Windows.UI.Composition.h>

using namespace winrt;
using namespace winrt::Windows::ApplicationModel::Core;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Foundation::Numerics;
using namespace winrt::Windows::Graphics::DirectX;
using namespace winrt::Windows::Graphics::DirectX::Direct3D11;
using namespace winrt::Windows::UI;
using namespace winrt::Windows::UI::Composition;
using namespace winrt::Windows::UI::Core;

namespace abi
{
	using namespace ABI::Windows::Foundation;
	using namespace ABI::Windows::Graphics::DirectX;
	using namespace ABI::Windows::UI::Composition;
}

// An app-provided helper to render lines of text.
struct SampleText
{


	SampleText(winrt::com_ptr<::IDWriteTextLayout> const& text, CompositionGraphicsDevice const& compositionGraphicsDevice, winrt::com_ptr<ID2D1Factory> const& d2dFactory, boolean outlineText
	//	, CustomTextRenderer* const& textRenderer
	) :
		m_textLayout(text)
		,m_compositionGraphicsDevice(compositionGraphicsDevice)
		, m_d2dFactory(d2dFactory)
		//,m_textRenderer(textRenderer)
	{
		// Create the surface just big enough to hold the formatted text block.
		DWRITE_TEXT_METRICS metrics;
		winrt::check_hresult(m_textLayout->GetMetrics(&metrics));
		winrt::Windows::Foundation::Size surfaceSize{ metrics.width, metrics.height };
	
		CompositionDrawingSurface drawingSurface{ m_compositionGraphicsDevice.CreateDrawingSurface(
			surfaceSize,
			DirectXPixelFormat::B8G8R8A8UIntNormalized,
			DirectXAlphaMode::Premultiplied) };

		// Cache the interop pointer, since that's what we always use.
		m_drawingSurfaceInterop = drawingSurface.as<abi::ICompositionDrawingSurfaceInterop>();

		// Draw the text
		//DrawText();
		//DrawTextWithEdgeDetectionEffect();
		if(outlineText)
			DrawOutlineText();
		else {
			DrawText();
		}
		// If the rendering device is lost, the application will recreate and replace it. We then
		// own redrawing our pixels.
		m_deviceReplacedEventToken = m_compositionGraphicsDevice.RenderingDeviceReplaced(
			[this](CompositionGraphicsDevice const&, RenderingDeviceReplacedEventArgs const&)
			{
				// Draw the text again.
				DrawText();
				return S_OK;
			});
		
	}

	~SampleText()
	{
		m_compositionGraphicsDevice.RenderingDeviceReplaced(m_deviceReplacedEventToken);
	}

	// Return the underlying surface to the caller.
	auto Surface()
	{
		// To the caller, the fact that we have a drawing surface is an implementation detail.
		// Return the base interface instead.
		return m_drawingSurfaceInterop.as<ICompositionSurface>();
	}

private:
	// The text to draw.
	winrt::com_ptr<::IDWriteTextLayout> m_textLayout;
	winrt::com_ptr<::ID2D1Factory> m_d2dFactory;


	// The composition surface that we use in the visual tree.
	winrt::com_ptr<abi::ICompositionDrawingSurfaceInterop> m_drawingSurfaceInterop;

	// The device that owns the surface.
	CompositionGraphicsDevice m_compositionGraphicsDevice{ nullptr };
	//winrt::com_ptr<abi::ICompositionGraphicsDevice> m_compositionGraphicsDevice2;

	// For managing our event notifier.
	winrt::event_token m_deviceReplacedEventToken;

	// We may detect device loss on BeginDraw calls. This helper handles this condition or other
	// errors.
	bool CheckForDeviceRemoved(HRESULT hr)
	{
		if (hr == S_OK)
		{
			// Everything is fine: go ahead and draw.
			return true;
		}

		if (hr == DXGI_ERROR_DEVICE_REMOVED)
		{
			// We can't draw at this time, but this failure is recoverable. Just skip drawing for
			// now. We will be asked to draw again once the Direct3D device is recreated.
			return false;
		}

		// Any other error is unexpected and, therefore, fatal.
		winrt::check_hresult(hr);
		return true;
	}

	// Renders the text into our composition surface
	void DrawText()
	{

		winrt::com_ptr<::ID2D1DeviceContext> m_d2dContext;
		// Begin our update of the surface pixels. If this is our first update, we are required
		// to specify the entire surface, which nullptr is shorthand for (but, as it works out,
		// any time we make an update we touch the entire surface, so we always pass nullptr).
		POINT offset;
		if (CheckForDeviceRemoved(m_drawingSurfaceInterop->BeginDraw(nullptr,
			__uuidof(ID2D1DeviceContext), m_d2dContext.put_void(), &offset)))
		{
			m_d2dContext->Clear(D2D1::ColorF(D2D1::ColorF::Black, 0.f));

			// Create a solid color brush for the text. A more sophisticated application might want
			// to cache and reuse a brush across all text elements instead, taking care to recreate
			// it in the event of device removed.
			winrt::com_ptr<::ID2D1SolidColorBrush> brush;
			winrt::check_hresult(m_d2dContext->CreateSolidColorBrush(
				D2D1::ColorF(D2D1::ColorF::White, 1.0f), brush.put()));

			// Draw the line of text at the specified offset, which corresponds to the top-left
			// corner of our drawing surface. Notice we don't call BeginDraw on the D2D device
			// context; this has already been done for us by the composition API.
			m_d2dContext->DrawTextLayout(D2D1::Point2F((float)offset.x, (float)offset.y), m_textLayout.get(),
				brush.get());

			// Our update is done. EndDraw never indicates rendering device removed, so any
			// failure here is unexpected and, therefore, fatal.
			winrt::check_hresult(m_drawingSurfaceInterop->EndDraw());
		}
	}

	// Renders the text into our composition surface
	void DrawOutlineText()
	{
		// Begin our update of the surface pixels. If this is our first update, we are required
		// to specify the entire surface, which nullptr is shorthand for (but, as it works out,
		// any time we make an update we touch the entire surface, so we always pass nullptr).
		winrt::com_ptr<::ID2D1DeviceContext> d2dDeviceContext;
		POINT offset;
		if (CheckForDeviceRemoved(m_drawingSurfaceInterop->BeginDraw(nullptr,
			__uuidof(ID2D1DeviceContext), d2dDeviceContext.put_void(), &offset)))
		{
			d2dDeviceContext->Clear(D2D1::ColorF(D2D1::ColorF::Black, 0.f));

			winrt::com_ptr<ID2D1SolidColorBrush> blackBrush;

			d2dDeviceContext->CreateSolidColorBrush(
				D2D1::ColorF(D2D1::ColorF::Black),
				blackBrush.put()
			);

			//setting the fillBrush to white, so we get the just the outline text.
			winrt::com_ptr<ID2D1SolidColorBrush> whiteBrush;

			d2dDeviceContext->CreateSolidColorBrush(
				D2D1::ColorF(D2D1::ColorF::White),
				whiteBrush.put()
			);


			CustomTextRenderer* textRenderer = new CustomTextRenderer(
				m_d2dFactory,
				d2dDeviceContext,
				blackBrush,
				whiteBrush
			);

			d2dDeviceContext->SetTransform(D2D1::Matrix3x2F::Identity());

			m_textLayout->Draw(
				d2dDeviceContext.get(),
				textRenderer,
				offset.x,
				offset.y
			);

			// Our update is done. EndDraw never indicates rendering device removed, so any
			// failure here is unexpected and, therefore, fatal.
			winrt::check_hresult(m_drawingSurfaceInterop->EndDraw());
		}
	}

	// Renders the text into our composition surface
	void DrawTextWithEdgeDetectionEffect()
	{
		// Begin our update of the surface pixels. If this is our first update, we are required
		// to specify the entire surface, which nullptr is shorthand for (but, as it works out,
		// any time we make an update we touch the entire surface, so we always pass nullptr).
		winrt::com_ptr<::ID2D1DeviceContext> d2dDeviceContext;
		POINT offset;
		if (CheckForDeviceRemoved(m_drawingSurfaceInterop->BeginDraw(nullptr,
			__uuidof(ID2D1DeviceContext), d2dDeviceContext.put_void(), &offset)))
		{
			d2dDeviceContext->Clear(D2D1::ColorF(D2D1::ColorF::Black, 0.f));
			winrt::com_ptr<::ID2D1SolidColorBrush> brush;
			winrt::check_hresult(d2dDeviceContext->CreateSolidColorBrush(
				D2D1::ColorF(D2D1::ColorF::White, 1.0f), brush.put()));

			winrt::com_ptr<ID2D1Image> originalTarget;
			d2dDeviceContext->GetTarget(originalTarget.put());

			//create command list
			winrt::com_ptr<ID2D1CommandList> commandList1;
			winrt::check_hresult(d2dDeviceContext->CreateCommandList(commandList1.put()));
			//Set the command list as the target
			d2dDeviceContext->SetTarget(commandList1.get());

			// Draw the line of text at the specified offset, which corresponds to the top-left
			// corner of our drawing surface. Notice we don't call BeginDraw on the D2D device
			// context; this has already been done for us by the composition API.
			d2dDeviceContext->DrawTextLayout(D2D1::Point2F((float)offset.x, (float)offset.y), m_textLayout.get(),
				brush.get());
			winrt::check_hresult(commandList1->Close());
			d2dDeviceContext->SetTarget(originalTarget.get());

			winrt::com_ptr<ID2D1Effect> edgeDetectionEffect;
			winrt::check_hresult(d2dDeviceContext->CreateEffect(CLSID_D2D1EdgeDetection, edgeDetectionEffect.put()));

			edgeDetectionEffect->SetInput(0, commandList1.get());
			edgeDetectionEffect->SetValue(D2D1_EDGEDETECTION_PROP_STRENGTH, 0.5f);
			edgeDetectionEffect->SetValue(D2D1_EDGEDETECTION_PROP_BLUR_RADIUS, 0.0f);
			edgeDetectionEffect->SetValue(D2D1_EDGEDETECTION_PROP_MODE, D2D1_EDGEDETECTION_MODE_SOBEL);
			edgeDetectionEffect->SetValue(D2D1_EDGEDETECTION_PROP_OVERLAY_EDGES, false);
			edgeDetectionEffect->SetValue(D2D1_EDGEDETECTION_PROP_ALPHA_MODE, D2D1_ALPHA_MODE_PREMULTIPLIED);

			d2dDeviceContext->DrawImage(edgeDetectionEffect.get());

			// Our update is done. EndDraw never indicates rendering device removed, so any
			// failure here is unexpected and, therefore, fatal.
			winrt::check_hresult(m_drawingSurfaceInterop->EndDraw());
		}
	}

};

struct DeviceLostEventArgs
{
	DeviceLostEventArgs(IDirect3DDevice const& device) : m_device(device) {}
	IDirect3DDevice Device() { return m_device; }
	static DeviceLostEventArgs Create(IDirect3DDevice const& device) { return DeviceLostEventArgs{ device }; }

private:
	IDirect3DDevice m_device;
};

struct DeviceLostHelper
{
	DeviceLostHelper() = default;

	~DeviceLostHelper()
	{
		StopWatchingCurrentDevice();
		m_onDeviceLostHandler = nullptr;
	}

	IDirect3DDevice CurrentlyWatchedDevice() { return m_device; }

	void WatchDevice(winrt::com_ptr<::IDXGIDevice> const& dxgiDevice)
	{
		// If we're currently listening to a device, then stop.
		StopWatchingCurrentDevice();

		// Set the current device to the new device.
		m_device = nullptr;
		winrt::check_hresult(::CreateDirect3D11DeviceFromDXGIDevice(dxgiDevice.get(), reinterpret_cast<::IInspectable**>(winrt::put_abi(m_device))));

		// Get the DXGI Device.
		m_dxgiDevice = dxgiDevice;

		// QI For the ID3D11Device4 interface.
		winrt::com_ptr<::ID3D11Device4> d3dDevice{ m_dxgiDevice.as<::ID3D11Device4>() };

		// Create a wait struct.
		m_onDeviceLostHandler = nullptr;
		m_onDeviceLostHandler = ::CreateThreadpoolWait(DeviceLostHelper::OnDeviceLost, (PVOID)this, nullptr);

		// Create a handle and a cookie.
		m_eventHandle = handle{ ::CreateEvent(nullptr, false, false, nullptr) };
		winrt::check_bool(bool{ m_eventHandle });
		m_cookie = 0;

		// Register for device lost.
		::SetThreadpoolWait(m_onDeviceLostHandler, m_eventHandle.get(), nullptr);
		winrt::check_hresult(d3dDevice->RegisterDeviceRemovedEvent(m_eventHandle.get(), &m_cookie));
	}

	void StopWatchingCurrentDevice()
	{
		if (m_dxgiDevice)
		{
			// QI For the ID3D11Device4 interface.
			auto d3dDevice{ m_dxgiDevice.as<::ID3D11Device4>() };

			// Unregister from the device lost event.
			::CloseThreadpoolWait(m_onDeviceLostHandler);
			d3dDevice->UnregisterDeviceRemoved(m_cookie);

			// Clear member variables.
			m_onDeviceLostHandler = nullptr;
			m_eventHandle.close();
			m_cookie = 0;
			m_device = nullptr;
		}
	}

	void DeviceLost(winrt::delegate<DeviceLostHelper const*, DeviceLostEventArgs const&> const& handler)
	{
		m_deviceLost = handler;
	}

	winrt::delegate<DeviceLostHelper const*, DeviceLostEventArgs const&> m_deviceLost;

private:
	void RaiseDeviceLostEvent(IDirect3DDevice const& oldDevice)
	{
		m_deviceLost(this, DeviceLostEventArgs::Create(oldDevice));
	}

	static void CALLBACK OnDeviceLost(PTP_CALLBACK_INSTANCE /* instance */, PVOID context, PTP_WAIT /* wait */, TP_WAIT_RESULT /* waitResult */)
	{
		auto deviceLostHelper = reinterpret_cast<DeviceLostHelper*>(context);
		auto oldDevice = deviceLostHelper->m_device;
		deviceLostHelper->StopWatchingCurrentDevice();
		deviceLostHelper->RaiseDeviceLostEvent(oldDevice);
	}

private:
	IDirect3DDevice m_device;
	winrt::com_ptr<::IDXGIDevice> m_dxgiDevice;
	PTP_WAIT m_onDeviceLostHandler{ nullptr };
	winrt::handle m_eventHandle;
	DWORD m_cookie{ 0 };
};

struct SampleApp : implements<SampleApp, IFrameworkViewSource, IFrameworkView>
{
	IFrameworkView CreateView()
	{
		return *this;
	}

	void Initialize(CoreApplicationView const &)
	{
	}

	// Run once when the application starts up
	void Initialize()
	{
		// Create a Direct2D device.
		CreateDirect2DDevice();

		// To create a composition graphics device, we need to QI for another interface
		winrt::com_ptr<abi::ICompositorInterop> compositorInterop{ m_compositor.as<abi::ICompositorInterop>() };

		// Create a graphics device backed by our D3D device
		winrt::com_ptr<abi::ICompositionGraphicsDevice> compositionGraphicsDeviceIface;
		winrt::check_hresult(compositorInterop->CreateGraphicsDevice(
			m_d2dDevice.get(),
			compositionGraphicsDeviceIface.put()));
		m_compositionGraphicsDevice = compositionGraphicsDeviceIface.as<CompositionGraphicsDevice>();
	}

	void Load(hstring const&)
	{
	}

	void Uninitialize()
	{
	}

	void Run()
	{
		CoreWindow window = CoreWindow::GetForCurrentThread();
		window.Activate();

		CoreDispatcher dispatcher = window.Dispatcher();
		dispatcher.ProcessEvents(CoreProcessEventsOption::ProcessUntilQuit);
	}

	void SetWindow(CoreWindow const& window)
	{
		m_compositor = Compositor{};
		m_target = m_compositor.CreateTargetForCurrentView();
		ContainerVisual root = m_compositor.CreateContainerVisual();
		m_target.Root(root);
		Rect windowBounds{ window.Bounds() };

		SpriteVisual viewport = m_compositor.CreateSpriteVisual();
		viewport.Brush(m_compositor.CreateColorBrush({ 0xFF, 0xEF, 0xE4 , 0xB0 }));
		viewport.Size({ windowBounds.Width,windowBounds.Height});
		//viewport.Size({ 0.0f + windowRect.right - windowRect.left, 0.0f + windowRect.bottom - windowRect.top });
		
		Initialize();

		winrt::check_hresult(
			::DWriteCreateFactory(
				DWRITE_FACTORY_TYPE_SHARED,
				__uuidof(m_dWriteFactory),
				reinterpret_cast<::IUnknown**>(m_dWriteFactory.put())
			)
		);

		winrt::check_hresult(
			m_dWriteFactory->CreateTextFormat(
				L"Bell MT",
				nullptr,
				DWRITE_FONT_WEIGHT_REGULAR,
				DWRITE_FONT_STYLE_NORMAL,
				DWRITE_FONT_STRETCH_NORMAL,
				48.0f,
				L"en-US",
				m_textFormat.put()
			)
		);

		winrt::check_hresult(
			m_dWriteFactory->CreateTextLayout(
				m_text.c_str(),
				(uint32_t)m_text.size(),
				m_textFormat.get(),
				windowBounds.Width,
				windowBounds.Height,
				m_textLayout.put()
			)
		);

		Visual shadowTextVisual{ CreateVisualFromTextLayout(m_textLayout, false) };
		shadowTextVisual.Size({ 200, 200 });
		shadowTextVisual.Offset({ 100 , 100, 0 });
		viewport.Children().InsertAtTop(shadowTextVisual);


		Visual outlineTextVisual{ CreateVisualFromTextLayout(m_textLayout, true) };
		outlineTextVisual.Size({ 200, 200 });
		outlineTextVisual.Offset({ 200 , 100, 0 });
		viewport.Children().InsertAtTop(outlineTextVisual);


		root.Children().InsertAtTop(viewport);

		AnimateVisual(shadowTextVisual);
	}

	void AnimateVisual(Visual visual)
	{
		auto animation = m_compositor.CreateVector3KeyFrameAnimation();
		animation.InsertKeyFrame(1.0f, { 200.0f, 100.0f, 0.0f });
		animation.Duration(std::chrono::seconds(2));
		animation.Direction(AnimationDirection::Alternate);
		// Run animation for 10 times
		animation.IterationCount(10);
		visual.StartAnimation(L"Offset", animation);
	}

	// Called when Direct3D signals the device lost event.
	void OnDirect3DDeviceLost(DeviceLostHelper const* /* sender */, DeviceLostEventArgs const& /* args */)
	{
		// Create a new Direct2D device.
		CreateDirect2DDevice();

		// Restore our composition graphics device to good health.
		winrt::com_ptr<abi::ICompositionGraphicsDeviceInterop> compositionGraphicsDeviceInterop{ m_compositionGraphicsDevice.as<abi::ICompositionGraphicsDeviceInterop>() };
		winrt::check_hresult(compositionGraphicsDeviceInterop->SetRenderingDevice(m_d2dDevice.get()));
	}

	// Create a surface that is asynchronously filled with an image
	ICompositionSurface CreateSurfaceFromTextLayout(winrt::com_ptr<::IDWriteTextLayout> const& text, boolean outlineText)
	{
		// Create our wrapper object that will handle downloading and decoding the image (assume
		// throwing new here).
		SampleText textSurface{ text, m_compositionGraphicsDevice, m_d2dFactory, outlineText };

		// The caller is only interested in the underlying surface.
		return textSurface.Surface();
	}

	// Create a visual that holds an image.
	Visual CreateVisualFromTextLayout(winrt::com_ptr<::IDWriteTextLayout> const& text, boolean outlineText)
	{
		// Create a sprite visual
		SpriteVisual spriteVisual{ m_compositor.CreateSpriteVisual() };


		// The sprite visual needs a brush to hold the image.
		CompositionSurfaceBrush surfaceBrush{
			m_compositor.CreateSurfaceBrush(CreateSurfaceFromTextLayout(text,outlineText))
		};

		// Associate the brush with the visual.
		CompositionBrush brush{ surfaceBrush.as<CompositionBrush>() };
		spriteVisual.Brush(brush);

		DropShadow textShadow{ m_compositor.CreateDropShadow() };

		textShadow.BlurRadius(4);
		textShadow.Mask(surfaceBrush);
		textShadow.Color(Colors::Black());
		textShadow.Offset({ 1.0f, 1.0f, 0.0f });
		spriteVisual.Shadow(textShadow);

		// Return the visual to the caller as an IVisual.
		return spriteVisual;
	}

private:
	CompositionTarget m_target{ nullptr };
	Compositor m_compositor{ nullptr };
	winrt::com_ptr<::ID2D1Device> m_d2dDevice;
	winrt::com_ptr<::IDXGIDevice> m_dxgiDevice;
	//winrt::com_ptr<abi::ICompositionGraphicsDevice> m_compositionGraphicsDevice;
	CompositionGraphicsDevice m_compositionGraphicsDevice{ nullptr };
	std::vector<SampleText> m_textSurfaces;
	DeviceLostHelper m_deviceLostHelper;
	winrt::com_ptr<::IDWriteFactory> m_dWriteFactory;
	winrt::com_ptr<::IDWriteTextFormat> m_textFormat;
	winrt::com_ptr<::IDWriteTextLayout> m_textLayout;
	std::wstring m_text{ L"Hello, World!" };
	winrt::com_ptr<::ID2D1DeviceContext> m_d2dContext;
	winrt::com_ptr<::ID2D1Factory1> m_d2dFactory;
	


	// This helper creates a Direct2D device, and registers for a device loss
	// notification on the underlying Direct3D device. When that notification is
	// raised, the OnDirect3DDeviceLost method is called.
	void CreateDirect2DDevice()
	{
		uint32_t createDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

		// Array with DirectX hardware feature levels in order of preference.
		D3D_FEATURE_LEVEL featureLevels[] =
		{
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_1,
			D3D_FEATURE_LEVEL_10_0,
			D3D_FEATURE_LEVEL_9_3,
			D3D_FEATURE_LEVEL_9_2,
			D3D_FEATURE_LEVEL_9_1
		};
		

		// Create the Direct3D 11 API device object and a corresponding context.
		winrt::com_ptr<::ID3D11Device> d3DDevice;
		winrt::com_ptr<::ID3D11DeviceContext> d3DImmediateContext;
		D3D_FEATURE_LEVEL d3dFeatureLevel{ D3D_FEATURE_LEVEL_9_1 };

		winrt::check_hresult(
			::D3D11CreateDevice(
				nullptr, // Default adapter.
				D3D_DRIVER_TYPE_HARDWARE,
				0, // Not asking for a software driver, so not passing a module to one.
				createDeviceFlags, // Set debug and Direct2D compatibility flags.
				featureLevels,
				ARRAYSIZE(featureLevels),
				D3D11_SDK_VERSION,
				d3DDevice.put(),
				&d3dFeatureLevel,
				d3DImmediateContext.put()
			)
		);

		// Initialize Direct2D resources.
		D2D1_FACTORY_OPTIONS d2d1FactoryOptions{ D2D1_DEBUG_LEVEL_NONE };

		// Initialize the Direct2D Factory.
		winrt::check_hresult(
			::D2D1CreateFactory(
				D2D1_FACTORY_TYPE_SINGLE_THREADED,
				__uuidof(m_d2dFactory),
				&d2d1FactoryOptions,
				m_d2dFactory.put_void()
			)
		);

		// Create the Direct2D device object.
		// Obtain the underlying DXGI device of the Direct3D device.
		m_dxgiDevice = d3DDevice.as<::IDXGIDevice>();

		m_d2dDevice = nullptr;
		winrt::check_hresult(
			m_d2dFactory->CreateDevice(m_dxgiDevice.get(), m_d2dDevice.put())
		);

		m_deviceLostHelper.WatchDevice(m_dxgiDevice);
		m_deviceLostHelper.DeviceLost({ this, &SampleApp::OnDirect3DDeviceLost });
	}
};

int __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
	CoreApplication::Run(SampleApp());
}