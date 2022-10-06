//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once
#include <functional>

namespace xamp {
namespace stream {
	class BassFileStream;
}
}

namespace xamp::stream::BassUtiltis {

void Encode(BassFileStream &stream, std::function<bool(uint32_t) > const& progress);

}

