//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/platfrom_handle.h>

class SingleInstanceApplication {
public:
    SingleInstanceApplication();

	~SingleInstanceApplication();

    bool attach() const;

private:
#ifdef XAMP_OS_WIN
	mutable WinHandle singular_;
#endif
};
