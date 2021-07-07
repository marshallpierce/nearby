// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// WARNING: Please don't edit this file. It was generated by C++/WinRT v2.0.210505.3

#ifndef WINRT_Windows_Perception_Spatial_Preview_1_H
#define WINRT_Windows_Perception_Spatial_Preview_1_H
#include "winrt/impl/Windows.Perception.Spatial.Preview.0.h"
WINRT_EXPORT namespace winrt::Windows::Perception::Spatial::Preview
{
    struct __declspec(empty_bases) ISpatialGraphInteropFrameOfReferencePreview :
        winrt::Windows::Foundation::IInspectable,
        impl::consume_t<ISpatialGraphInteropFrameOfReferencePreview>
    {
        ISpatialGraphInteropFrameOfReferencePreview(std::nullptr_t = nullptr) noexcept {}
        ISpatialGraphInteropFrameOfReferencePreview(void* ptr, take_ownership_from_abi_t) noexcept : winrt::Windows::Foundation::IInspectable(ptr, take_ownership_from_abi) {}
        ISpatialGraphInteropFrameOfReferencePreview(ISpatialGraphInteropFrameOfReferencePreview const&) noexcept = default;
        ISpatialGraphInteropFrameOfReferencePreview(ISpatialGraphInteropFrameOfReferencePreview&&) noexcept = default;
        ISpatialGraphInteropFrameOfReferencePreview& operator=(ISpatialGraphInteropFrameOfReferencePreview const&) & noexcept = default;
        ISpatialGraphInteropFrameOfReferencePreview& operator=(ISpatialGraphInteropFrameOfReferencePreview&&) & noexcept = default;
    };
    struct __declspec(empty_bases) ISpatialGraphInteropPreviewStatics :
        winrt::Windows::Foundation::IInspectable,
        impl::consume_t<ISpatialGraphInteropPreviewStatics>
    {
        ISpatialGraphInteropPreviewStatics(std::nullptr_t = nullptr) noexcept {}
        ISpatialGraphInteropPreviewStatics(void* ptr, take_ownership_from_abi_t) noexcept : winrt::Windows::Foundation::IInspectable(ptr, take_ownership_from_abi) {}
        ISpatialGraphInteropPreviewStatics(ISpatialGraphInteropPreviewStatics const&) noexcept = default;
        ISpatialGraphInteropPreviewStatics(ISpatialGraphInteropPreviewStatics&&) noexcept = default;
        ISpatialGraphInteropPreviewStatics& operator=(ISpatialGraphInteropPreviewStatics const&) & noexcept = default;
        ISpatialGraphInteropPreviewStatics& operator=(ISpatialGraphInteropPreviewStatics&&) & noexcept = default;
    };
    struct __declspec(empty_bases) ISpatialGraphInteropPreviewStatics2 :
        winrt::Windows::Foundation::IInspectable,
        impl::consume_t<ISpatialGraphInteropPreviewStatics2>
    {
        ISpatialGraphInteropPreviewStatics2(std::nullptr_t = nullptr) noexcept {}
        ISpatialGraphInteropPreviewStatics2(void* ptr, take_ownership_from_abi_t) noexcept : winrt::Windows::Foundation::IInspectable(ptr, take_ownership_from_abi) {}
        ISpatialGraphInteropPreviewStatics2(ISpatialGraphInteropPreviewStatics2 const&) noexcept = default;
        ISpatialGraphInteropPreviewStatics2(ISpatialGraphInteropPreviewStatics2&&) noexcept = default;
        ISpatialGraphInteropPreviewStatics2& operator=(ISpatialGraphInteropPreviewStatics2 const&) & noexcept = default;
        ISpatialGraphInteropPreviewStatics2& operator=(ISpatialGraphInteropPreviewStatics2&&) & noexcept = default;
    };
}
#endif