//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>

#include <base/base.h>
#include <base/memory.h>

XAMP_BASE_NAMESPACE_BEGIN

class XAMP_BASE_API Transliterator final {
public:
    Transliterator();

    XAMP_PIMPL(Transliterator)

    std::string TransformToLatin(const std::wstring& name);

    char GetLatinLetter(const std::wstring& name);
private:
    class TransliteratorImpl;
    AlignPtr<TransliteratorImpl> impl_;
};

XAMP_BASE_NAMESPACE_END