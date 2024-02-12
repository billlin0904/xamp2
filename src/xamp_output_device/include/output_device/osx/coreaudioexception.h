//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_MAC

#include <base/exception.h>
#include <base/logger.h>
#include <CoreAudio/CoreAudio.h>

namespace xamp::output_device::osx {

class CoreAudioException final : public base::Exception {
public:
    explicit CoreAudioException(OSStatus status);

    static std::string ErrorToString(OSStatus status);
};

#define CoreAudioThrowIfError(err) \
do { \
     if ((err) != noErr) { \
        throw CoreAudioException(err); \
    } \
} while (false)

#define CoreAudioFailedLog(expr) \
    do {\
        auto err = expr;\
        if (err != noErr) {\
            XAMP_LOG_DEBUG(CoreAudioException::ErrorToString(err));\
        }\
    } while(false)


}

#endif

