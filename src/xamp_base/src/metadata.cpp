#include <base/metadata.h>

namespace xamp::base {

Metadata::Metadata() noexcept
	: track(0)
	, bitrate(0)
	, samplerate(0)
	, timestamp(0)
	, offset(0)
	, duration(0) {
}

}
