//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

namespace xamp::base {

class StackTrace {
public:
    StackTrace();

private:
    static void AbortHandler(int32_t signum);
};

}


