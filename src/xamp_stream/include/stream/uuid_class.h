//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>
#include <base/uuid.h>
#include <base/uuidof.h>

XAMP_STREAM_NAMESPACE_BEGIN

#define XAMP_DECLARE_UUID_CLASS(ClassName) \
    public:\
    constexpr static auto Description = std::string_view(#ClassName); \
    \
    [[nodiscard]] Uuid GetTypeId() const override { \
        return XAMP_UUID_OF(ClassName); \
    } \
    \
    [[nodiscard]] std::string_view GetDescription() const noexcept override { \
        return Description; \
    }

class XAMP_NO_VTABLE XAMP_STREAM_API IUUIDClass {
public:
    XAMP_BASE_CLASS(IUUIDClass)

    /*
     * Get the type id of the audio processor.
     *
     * @return the type id of the audio processor.
     */
    [[nodiscard]] virtual Uuid GetTypeId() const = 0;

    /*
     * Get the description of the audio processor.
     *
     * @return the description of the audio processor.
     */
    [[nodiscard]] virtual std::string_view GetDescription() const noexcept = 0;

protected:
    IUUIDClass() = default;
};


XAMP_STREAM_NAMESPACE_END

