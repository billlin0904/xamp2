//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/exception.h>
#include <stream/stream.h>

XAMP_STREAM_NAMESPACE_BEGIN

class XAMP_STREAM_API BassException final : public Exception {
public:
    BassException();

    explicit BassException(int error);
	
private:
    int error_;
};

#define BassIfFailedThrow(cond) \
	do {\
        if (!(cond)) {\
            throw BassException();\
        }\
    } while (false)

XAMP_STREAM_NAMESPACE_END
