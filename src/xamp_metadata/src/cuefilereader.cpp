#include <fstream>
#include <metadata/api.h>
#include <metadata/cueloader.h>
#include <metadata/libcuelib.h>
#include <base/fs.h>
#include <base/fastmutex.h>
#include <base/dll.h>
#include <base/logger.h>
#include <base/logger_impl.h>

extern "C" {
#include <libcue.h>
}

XAMP_METADATA_NAMESPACE_BEGIN

namespace {
	bool IsYear(const char* s) {
		auto is_digit = [](char c)
			{ return c >= '0' && c <= '9'; };

		return is_digit(s[0]) && is_digit(s[1]) &&
			is_digit(s[2]) && is_digit(s[3]) && !s[4];
	}

	double FrameToSecond(long frame) {
		int m = 0; 
		int s = 0;
		int f = 0;
		f = frame % 75;
		frame /= 75;
		s = frame % 60;
		frame /= 60;
		m = frame;
		double total = m * 60 + s + f / 75.0;
		return total;
	}

	long SecondToFrame(double second) {
		return second * 75;
	}

	FastMutex libcue_mutex;
}

XAMP_DECLARE_LOG_NAME(CueLoader);

class CueLoader::CueLoaderImpl {
public:
	CueLoaderImpl() 
		: logger_(XampLoggerFactory.GetLogger(kCueLoaderLoggerName)) {
	}

	std::vector<TrackInfo> Load(const Path& path) {
		std::vector<TrackInfo> track_infos;

		auto utf8_text = ReadFileToUtf8String(path);

		// NOTE: libcue is not thread-safe so we need to lock it.
		std::lock_guard<FastMutex> lock{ libcue_mutex };

		auto *cd_handle = LIBCUE_LIB.cue_parse_string(utf8_text.c_str());
		if (!cd_handle) {
			XAMP_LOG_E(logger_, "failed to parse cue file.");
			return track_infos;
		}

		CdPtr cd(cd_handle);

		std::optional<TrackInfo> opt_track_info;
		TrackInfo track_info;
		std::wstring album;

		auto* cd_text = LIBCUE_LIB.cd_get_cdtext(cd.get());
		if (cd_text != nullptr) {
			const char* s = nullptr;
			if ((s = LIBCUE_LIB.cdtext_get(PTI_TITLE, cd_text)))
				album = String::ToString(s);
		}
				
		auto tracks = LIBCUE_LIB.cd_get_ntrack(cd.get());
		auto* cur = LIBCUE_LIB.cd_get_track(cd.get(), 1);
		const char* cur_name = cur ? LIBCUE_LIB.track_get_filename(cur) : nullptr;
		if (!cur_name) {
			XAMP_LOG_E(logger_, "failed to parse cue file.");
			return track_infos;
		}
		
		bool same_file = false;
		double file_duration = 0;
		for (int track = 1; track <= tracks; track++) {
			if (!same_file) {
				auto reader = MakeMetadataReader();
				auto wide_cur_name = String::ToString(cur_name);
				if (path.has_root_path()) {
					auto file_path = path.parent_path() / Path(wide_cur_name);
					track_info = reader->Extract(file_path);
					file_duration = track_info.duration;
				}
				else {
					track_info = reader->Extract(Path(wide_cur_name));
					file_duration = track_info.duration;
				}		
				track_info.is_cue_file = true;
				track_info.album = album;
				auto* cd_text = LIBCUE_LIB.cd_get_cdtext(cd.get());
				if (cd_text != nullptr) {
					GetCdText(cd_text, track_info);
				}
				auto* rem = LIBCUE_LIB.cd_get_rem(cd.get());
				if (rem != nullptr) {
					GetYear(rem, track_info);
					GetReplayGain(rem, track_info);
				}
				opt_track_info = track_info;
			}
			
			auto* next = (track + 1 <= tracks) ? LIBCUE_LIB.cd_get_track(cd.get(), track + 1) : nullptr;
			const char* next_name = next ? LIBCUE_LIB.track_get_filename(next) : nullptr;
			same_file = (next_name && !strcmp(next_name, cur_name));

			if (auto track_info = opt_track_info) {
				auto wide_cur_name = !cur_name ? L"" : String::ToString(cur_name);
				auto file_path = path.parent_path() / Path(wide_cur_name);
				track_info.value().file_path = file_path;

				auto begin = LIBCUE_LIB.track_get_start(cur);
				track_info.value().offset = FrameToSecond(begin);

				if (same_file) {
					auto length = LIBCUE_LIB.track_get_length(cur);
					track_info.value().duration = FrameToSecond(length);
				} else { 
					auto length = SecondToFrame(file_duration);
					track_info.value().duration = FrameToSecond(length - begin);
				}

				auto* cd_text = LIBCUE_LIB.track_get_cdtext(cur);
				if (cd_text != nullptr) {
					GetCdText(cd_text, track_info.value());
				}

				auto* rem = LIBCUE_LIB.track_get_rem(cur);
				if (rem) {
					GetReplayGain(rem, track_info.value());
				}
				track_info.value().track = track;
				track_infos.push_back(track_info.value());
			}

			if (!next_name) {
				break;
			}

			cur = next;
			cur_name = next_name;
		}
		return track_infos;
	}

	void GetYear(Rem* rem, TrackInfo& info) {
		const char* s;
		if ((s = LIBCUE_LIB.rem_get(REM_DATE, rem))) {
			if (IsYear(s))
				info.year = std::atoi(s);
			else
				info.year = 0;
		}
	}

	void GetReplayGain(Rem* rem, TrackInfo& info) {
		const char* s = nullptr;		
		ReplayGain replay_gain;
		if ((s = LIBCUE_LIB.rem_get(REM_REPLAYGAIN_ALBUM_GAIN, rem)))
			replay_gain.album_gain = std::atof(s);
		if ((s = LIBCUE_LIB.rem_get(REM_REPLAYGAIN_ALBUM_PEAK, rem)))
			replay_gain.album_peak = std::atof(s);
		if ((s = LIBCUE_LIB.rem_get(REM_REPLAYGAIN_TRACK_GAIN, rem)))
			replay_gain.track_gain = std::atof(s);
		if ((s = LIBCUE_LIB.rem_get(REM_REPLAYGAIN_TRACK_PEAK, rem)))
			replay_gain.track_peak = std::atof(s);
		info.replay_gain = replay_gain;
	}

	void GetCdText(Cdtext* cd_text, TrackInfo& track_info) {
		const char* s = nullptr;
		if ((s = LIBCUE_LIB.cdtext_get(PTI_PERFORMER, cd_text)))
			track_info.artist = String::ToString(s);
		if ((s = LIBCUE_LIB.cdtext_get(PTI_TITLE, cd_text)))
			track_info.title = String::ToString(s);
		if ((s = LIBCUE_LIB.cdtext_get(PTI_GENRE, cd_text)))
			track_info.genre = String::ToString(s);
		if ((s = LIBCUE_LIB.cdtext_get(PTI_COMPOSER, cd_text)))
			track_info.artist = String::ToString(s);
	}

private:
	template <typename T>
	struct CdPtrDeleter;

	template <>
	struct CdPtrDeleter<Cd> {
		void operator()(Cd* p) const {
			XAMP_EXPECTS(p != nullptr);
			LIBCUE_LIB.cd_delete(p);
		}
	};

	using CdPtr = std::unique_ptr<Cd, CdPtrDeleter<Cd>>;

	LoggerPtr logger_;
};

CueLoader::CueLoader()
	: impl_(MakeAlign<CueLoaderImpl>()) {
}

XAMP_PIMPL_IMPL(CueLoader)

std::vector<TrackInfo> CueLoader::Load(const Path& path) {
	return impl_->Load(path);
}

XAMP_METADATA_NAMESPACE_END
