#include <fstream>
#include <metadata/api.h>
#include <metadata/cueloader.h>
#include <base/dll.h>
#include <base/logger.h>
#include <base/logger_impl.h>

extern "C" {
#include <libcue.h>
}

XAMP_METADATA_NAMESPACE_BEGIN

namespace {
	class LibCueLib final {
	public:
		LibCueLib();

		XAMP_DISABLE_COPY(LibCueLib)

	private:
		SharedLibraryHandle module_;

	public:
		XAMP_DECLARE_DLL_NAME(cue_parse_string);
		XAMP_DECLARE_DLL_NAME(cd_get_ntrack);
		XAMP_DECLARE_DLL_NAME(cd_get_track);
		XAMP_DECLARE_DLL_NAME(track_get_filename);
		XAMP_DECLARE_DLL_NAME(cd_get_cdtext);
		XAMP_DECLARE_DLL_NAME(cdtext_get);
		XAMP_DECLARE_DLL_NAME(rem_get);
		XAMP_DECLARE_DLL_NAME(cd_get_rem);
		XAMP_DECLARE_DLL_NAME(track_get_start);
		XAMP_DECLARE_DLL_NAME(track_get_cdtext);
		XAMP_DECLARE_DLL_NAME(track_get_rem);
	};

#define LIBCUE_LIB Singleton<LibCueLib>::GetInstance()

	LibCueLib::LibCueLib() try
		: module_(OpenSharedLibrary("libcue"))
		, XAMP_LOAD_DLL_API(cue_parse_string)
		, XAMP_LOAD_DLL_API(cd_get_ntrack)
		, XAMP_LOAD_DLL_API(cd_get_track)
		, XAMP_LOAD_DLL_API(track_get_filename)
		, XAMP_LOAD_DLL_API(cd_get_cdtext)
		, XAMP_LOAD_DLL_API(cdtext_get)
		, XAMP_LOAD_DLL_API(rem_get)
		, XAMP_LOAD_DLL_API(cd_get_rem)
		, XAMP_LOAD_DLL_API(track_get_start)
		, XAMP_LOAD_DLL_API(track_get_cdtext)
		, XAMP_LOAD_DLL_API(track_get_rem) {
	}
	catch (const Exception& e) {
		XAMP_LOG_ERROR("{}", e.GetErrorMessage());
	}

	bool IsYear(const char* s) {
		auto is_digit = [](char c)
			{ return c >= '0' && c <= '9'; };

		return is_digit(s[0]) && is_digit(s[1]) &&
			is_digit(s[2]) && is_digit(s[3]) && !s[4];
	}
}

XAMP_DECLARE_LOG_NAME(CueLoader);

class CueLoader::CueLoaderImpl {
public:
	CueLoaderImpl() 
		: logger_(XampLoggerFactory.GetLogger(kCueLoaderLoggerName)) {
	}

	std::vector<TrackInfo> Load(const Path& path) {
		std::vector<TrackInfo> track_infos;

		std::ifstream file;
		file.open(path);
		if (!file.is_open()) {
			XAMP_LOG_E(logger_, "Failed to open cue file: {}", path);
			return track_infos;
		}

		std::string all(std::istreambuf_iterator<char>(file),
			(std::istreambuf_iterator<char>()));
		auto *cd = LIBCUE_LIB.cue_parse_string(all.c_str());
		if (!cd) {
			XAMP_LOG_E(logger_, "Failed to parse cue file: {}", path);
			return track_infos;
		}
				
		auto tracks = LIBCUE_LIB.cd_get_ntrack(cd);
		auto* cur = LIBCUE_LIB.cd_get_track(cd, 1);
		const char* cur_name = cur ? LIBCUE_LIB.track_get_filename(cur) : nullptr;
		if (!cur_name) {
			return track_infos;
		}

		std::optional<TrackInfo> opt_track_info;
		bool same_file = false;
		for (int track = 1; track <= tracks; track++) {
			if (!same_file) {
				auto reader = MakeMetadataReader();
				TrackInfo info;

				if (path.has_root_path()) {
					info = reader->Extract(path.parent_path() / Path(cur_name));
				}
				else {
					info = reader->Extract(Path(cur_name));
				}

				auto* cd_text = LIBCUE_LIB.cd_get_cdtext(cd);
				if (cd_text != nullptr) {
					GetCdText(cd_text, info);
				}

				auto* rem = LIBCUE_LIB.cd_get_rem(cd);
				if (rem != nullptr) {
					GetYear(rem, info);
					GetReplayGain(rem, info);
				}

				opt_track_info = info;
			}
			
			auto* next = (track + 1 <= tracks)
				? LIBCUE_LIB.cd_get_track(cd, track + 1) : nullptr;
			const char* next_name = next ? LIBCUE_LIB.track_get_filename(next) : nullptr;
			same_file = (next_name && !strcmp(next_name, cur_name));

			if (auto track_info = opt_track_info) {
				auto file_path = path / Path(cur_name);
				track_info.value().file_path = file_path;

				auto begin = LIBCUE_LIB.track_get_start(cur) * 1000.0 / 75.0;
				if (same_file) {
					auto end = LIBCUE_LIB.track_get_start(next) * 1000.0 / 75.0;
					track_info.value().duration = end - begin;
				} else { 
					auto length = track_info.value().duration;
					if (length > 0) {
						track_info.value().duration = length - begin;
					}
				}

				auto* cd_text = LIBCUE_LIB.track_get_cdtext(cur);
				if (cd_text != nullptr) {
					GetCdText(cd_text, track_info.value());
				}

				auto* rem = LIBCUE_LIB.track_get_rem(cur);
				if (rem) {
					GetReplayGain(rem, track_info.value());
				}

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

	void GetYear(Rem* rem, xamp::base::TrackInfo& info) {
		const char* s;
		if ((s = LIBCUE_LIB.rem_get(REM_DATE, rem))) {
			if (IsYear(s))
				info.year = 0;
			else
				info.year = 0;
		}
	}

	void GetReplayGain(Rem* rem, TrackInfo& info) {
		const char* s = nullptr;
		ReplayGain replay_gain;
		if ((s = LIBCUE_LIB.rem_get(REM_REPLAYGAIN_ALBUM_GAIN, rem)))
			info.replay_gain = replay_gain;
		if ((s = LIBCUE_LIB.rem_get(REM_REPLAYGAIN_ALBUM_PEAK, rem)))
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
