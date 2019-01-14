//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved

#pragma once

////  CustomTextRenderer
////
////  The IDWriteTextRenderer interface is an input parameter to
////  IDWriteTextLayout::Draw.  This interfaces defines a number of
////  callback functions that the client application implements for
////  custom text rendering.  This sample renderer implementation
////  renders text using text outlines and Direct2D.
////  A more sophisticated client would also support bitmap
////  renderings.

class CustomTextRenderer : public IDWriteTextRenderer
{
public:
    CustomTextRenderer(
        winrt::com_ptr<ID2D1Factory> D2DFactory,
        winrt::com_ptr<ID2D1DeviceContext> D2DDeviceContext,
        winrt::com_ptr<ID2D1SolidColorBrush> outlineBrush,
		winrt::com_ptr<ID2D1SolidColorBrush> fillBrush
        );

    IFACEMETHOD(IsPixelSnappingDisabled)(
        _In_opt_ void* clientDrawingContext,
        _Out_ BOOL* isDisabled
        );

    IFACEMETHOD(GetCurrentTransform)(
        _In_opt_ void* clientDrawingContext,
        _Out_ DWRITE_MATRIX* transform
        );

    IFACEMETHOD(GetPixelsPerDip)(
        _In_opt_ void* clientDrawingContext,
        _Out_ FLOAT* pixelsPerDip
        );

    IFACEMETHOD(DrawGlyphRun)(
        _In_opt_ void* clientDrawingContext,
        FLOAT baselineOriginX,
        FLOAT baselineOriginY,
        DWRITE_MEASURING_MODE measuringMode,
        _In_ DWRITE_GLYPH_RUN const* glyphRun,
        _In_ DWRITE_GLYPH_RUN_DESCRIPTION const* glyphRunDescription,
        IUnknown* clientDrawingEffect
        );

    IFACEMETHOD(DrawUnderline)(
        _In_opt_ void* clientDrawingContext,
        FLOAT baselineOriginX,
        FLOAT baselineOriginY,
        _In_ DWRITE_UNDERLINE const* underline,
        IUnknown* clientDrawingEffect
        );

    IFACEMETHOD(DrawStrikethrough)(
        _In_opt_ void* clientDrawingContext,
        FLOAT baselineOriginX,
        FLOAT baselineOriginY,
        _In_ DWRITE_STRIKETHROUGH const* strikethrough,
        IUnknown* clientDrawingEffect
        );

    IFACEMETHOD(DrawInlineObject)(
        _In_opt_ void* clientDrawingContext,
        FLOAT originX,
        FLOAT originY,
        IDWriteInlineObject* inlineObject,
        BOOL isSideways,
        BOOL isRightToLeft,
        IUnknown* clientDrawingEffect
        );

public:
    IFACEMETHOD_(unsigned long, AddRef) ();
    IFACEMETHOD_(unsigned long, Release) ();
    IFACEMETHOD(QueryInterface) (
        IID const& riid,
        void** ppvObject
        );

private:
    unsigned long                                             refCount;
    winrt::com_ptr<ID2D1Factory>                              D2DFactory;
    winrt::com_ptr<ID2D1DeviceContext>                        D2DDeviceContext;
    winrt::com_ptr<ID2D1SolidColorBrush>                      outlineBrush;
	winrt::com_ptr<ID2D1SolidColorBrush>                      fillBrush;
};
