#include <base/stl.h>
#include <base/trackinfo.h>

XAMP_BASE_NAMESPACE_BEGIN

namespace {
	template <typename Resolution = std::chrono::seconds>
	time_t getTime_t() {
		return std::chrono::duration_cast<Resolution>(
			std::chrono::system_clock::now().time_since_epoch()).count();
	}
}

TrackInfo::TrackInfo() : is_cue_file(0)
	, rating(false)
	, track(0)
	, bit_rate(0)
	, sample_rate(0)
	, year(0)
	, file_size(0)
	, last_write_time(getTime_t())
	, duration(0)
	, offset(0) {
}

XAMP_BASE_NAMESPACE_END
