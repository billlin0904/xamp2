//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>

#include <base/base.h>
#include <base/memory.h>

XAMP_BASE_NAMESPACE_BEGIN

enum class LanguageType {
    LANGUAGE_ENGLISH,
    LANGUAGE_CHINESE,
    LANGUAGE_JAPANESE,
    LANGUAGE_UNKNOWN
};

class XAMP_BASE_API NameConverter final {
public:
    NameConverter();

    XAMP_PIMPL(NameConverter)

    std::string ConvertName(const std::wstring& name, LanguageType lang);

    char GetInitialLetter(const std::wstring& name, LanguageType lang);
private:
    class NameConverterImpl;
    AlignPtr<NameConverterImpl> impl_;
};

XAMP_BASE_NAMESPACE_END