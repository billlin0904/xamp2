// This is an open source non-commercial project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <base/metadata.h>

namespace xamp::base {

Metadata::Metadata() noexcept
	: track(0)
	, bitrate(0)
	, samplerate(0)
	, offset(0)
	, duration(0) {
}

}
