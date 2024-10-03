#include <base/stl.h>
#include <base/trackinfo.h>
#include <base/algorithm.h>

XAMP_BASE_NAMESPACE_BEGIN

TrackInfo::TrackInfo() noexcept
	: is_cue_file(false)
	, rating(false)
	, track(0)
	, bit_rate(0)
	, sample_rate(0)
	, year(0)
	, file_size(0)
	, last_write_time(GetTime_t())
	, duration(0)
	, offset(0) {
}

XAMP_BASE_NAMESPACE_END
