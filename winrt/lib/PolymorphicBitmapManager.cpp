// Copyright (c) Microsoft Corporation. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may
// not use these files except in compliance with the License. You may obtain
// a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.

#include "pch.h"

#include "PolymorphicBitmapManager.h"
#include "CanvasBitmap.h"
#include "CanvasRenderTarget.h"

namespace ABI { namespace Microsoft { namespace Graphics { namespace Canvas
{
    class DefaultBitmapResourceCreationAdapter : public ICanvasBitmapResourceCreationAdapter
    {
        ComPtr<IWICImagingFactory2> m_wicFactory;

    public:
        DefaultBitmapResourceCreationAdapter()
        {
            ThrowIfFailed(
                CoCreateInstance(
                CLSID_WICImagingFactory,
                nullptr,
                CLSCTX_INPROC_SERVER,
                IID_PPV_ARGS(&m_wicFactory)));
        }

        ComPtr<IWICFormatConverter> CreateWICFormatConverter(HSTRING fileName)
        {
            ComPtr<IWICFormatConverter> wicFormatConverter;

            wchar_t const* fileNameBuffer = WindowsGetStringRawBuffer(fileName, nullptr);

            if (!fileNameBuffer || wcslen(fileNameBuffer)==0)
                ThrowHR(E_INVALIDARG);

            ComPtr<IWICBitmapDecoder> wicBitmapDecoder;
            ThrowIfFailed(m_wicFactory->CreateDecoderFromFilename(
                fileNameBuffer, 
                nullptr, 
                GENERIC_READ, 
                WICDecodeMetadataCacheOnLoad, 
                &wicBitmapDecoder));

            ComPtr<IWICBitmapFrameDecode> wicBitmapFrameSource;
            ThrowIfFailed(wicBitmapDecoder->GetFrame(0, &wicBitmapFrameSource));

            ThrowIfFailed(m_wicFactory->CreateFormatConverter(&wicFormatConverter));

            ThrowIfFailed(wicFormatConverter->Initialize(
                wicBitmapFrameSource.Get(), 
                GUID_WICPixelFormat32bppPBGRA, 
                WICBitmapDitherTypeNone, 
                NULL, 
                0, 
                WICBitmapPaletteTypeMedianCut));

            return wicFormatConverter;
        }
    };


    PolymorphicBitmapManager::PolymorphicBitmapManager(std::shared_ptr<ICanvasBitmapResourceCreationAdapter> adapter)
        : m_bitmapManager(std::make_shared<CanvasBitmapManager>(adapter))
        , m_renderTargetManager(std::make_shared<CanvasRenderTargetManager>())
    {
    }


    ComPtr<ICanvasBitmap> PolymorphicBitmapManager::GetOrCreateBitmap(
        ICanvasDevice* device, 
        ID2D1Bitmap1* d2dBitmap)
    {
        if (IsRenderTargetBitmap(d2dBitmap))
            return m_renderTargetManager->GetOrCreate(device, d2dBitmap);
        else
            return m_bitmapManager->GetOrCreate(d2dBitmap);
    }


    ComPtr<CanvasRenderTarget> PolymorphicBitmapManager::GetOrCreateRenderTarget(
        ICanvasDevice* device, 
        ID2D1Bitmap1* d2dBitmap)
    {
        if (IsRenderTargetBitmap(d2dBitmap))
            return m_renderTargetManager->GetOrCreate(device, d2dBitmap);
        else
            ThrowHR(E_INVALIDARG);
    }


    std::shared_ptr<PolymorphicBitmapManager> PerApplicationPolymorphicBitmapManager::CreateManager()
    {
        auto adapter = std::make_shared<DefaultBitmapResourceCreationAdapter>();
        return std::make_shared<PolymorphicBitmapManager>(adapter);
    }


}}}}