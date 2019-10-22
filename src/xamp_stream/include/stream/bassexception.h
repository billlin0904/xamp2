//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#include <base/exception.h>
#include <stream/stream.h>

namespace xamp::stream {

using namespace base;

class XAMP_STREAM_API BassException final : public Exception {
public:
    explicit BassException(int error);

    ~BassException() override;
private:
    int error_;
};

}
