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

#ifndef WINRT_Windows_Globalization_Collation_2_H
#define WINRT_Windows_Globalization_Collation_2_H
#include "winrt/impl/Windows.Foundation.Collections.1.h"
#include "winrt/impl/Windows.Globalization.Collation.1.h"
WINRT_EXPORT namespace winrt::Windows::Globalization::Collation
{
    struct __declspec(empty_bases) CharacterGrouping : winrt::Windows::Globalization::Collation::ICharacterGrouping
    {
        CharacterGrouping(std::nullptr_t) noexcept {}
        CharacterGrouping(void* ptr, take_ownership_from_abi_t) noexcept : winrt::Windows::Globalization::Collation::ICharacterGrouping(ptr, take_ownership_from_abi) {}
        CharacterGrouping(CharacterGrouping const&) noexcept = default;
        CharacterGrouping(CharacterGrouping&&) noexcept = default;
        CharacterGrouping& operator=(CharacterGrouping const&) & noexcept = default;
        CharacterGrouping& operator=(CharacterGrouping&&) & noexcept = default;
    };
    struct __declspec(empty_bases) CharacterGroupings : winrt::Windows::Globalization::Collation::ICharacterGroupings
    {
        CharacterGroupings(std::nullptr_t) noexcept {}
        CharacterGroupings(void* ptr, take_ownership_from_abi_t) noexcept : winrt::Windows::Globalization::Collation::ICharacterGroupings(ptr, take_ownership_from_abi) {}
        CharacterGroupings();
        explicit CharacterGroupings(param::hstring const& language);
        CharacterGroupings(CharacterGroupings const&) noexcept = default;
        CharacterGroupings(CharacterGroupings&&) noexcept = default;
        CharacterGroupings& operator=(CharacterGroupings const&) & noexcept = default;
        CharacterGroupings& operator=(CharacterGroupings&&) & noexcept = default;
    };
}
#endif