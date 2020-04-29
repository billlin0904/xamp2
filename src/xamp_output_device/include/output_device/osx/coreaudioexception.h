//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#include <base/exception.h>
#include <base/logger.h>
#include <CoreAudio/CoreAudio.h>

namespace xamp::output_device::osx {

using namespace base;

class CoreAudioException final : public Exception {
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

#define CoreAudioFailledLog(expr) \
    do {\
        auto err = expr;\
        if (err != kAudioHardwareNoError) {\
            XAMP_LOG_DEBUG("{}", CoreAudioException::ErrorToString(err));\
        }\
    } while(false)


}

