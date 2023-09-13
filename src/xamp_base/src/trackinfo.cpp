#include <base/stl.h>
#include <base/trackinfo.h>

namespace xamp::base {

TrackInfo::TrackInfo() noexcept
	: track(0)
	, bit_rate(0)
	, sample_rate(0)
	, year(0)
	, file_size(0)
	, last_write_time(GetTime_t())
	//, parent_path_hash(0)
	, offset(0)
	, duration(0) {
}

}
