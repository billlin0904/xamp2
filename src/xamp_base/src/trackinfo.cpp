#include <base/trackinfo.h>

namespace xamp::base {

TrackInfo::TrackInfo() noexcept
	: track(0)
	, bitrate(0)
	, samplerate(0)
	, year(0)
	, last_write_time(0)
	, file_size(0)
	, parent_path_hash(0)
	, offset(0)
	, duration(0) {
}

}
