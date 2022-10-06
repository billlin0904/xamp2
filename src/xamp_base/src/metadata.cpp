#include <base/metadata.h>

namespace xamp::base {

Metadata::Metadata() noexcept
	: track(0)
	, bitrate(0)
	, samplerate(0)
	, year(0)
	, last_write_time(0)
	, file_size(0)
	, offset(0)
	, duration(0) {
}

}
