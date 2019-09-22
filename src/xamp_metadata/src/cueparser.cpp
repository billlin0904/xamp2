#if 0
#include <sstream>
#include <array>

#include <base/bom.h>

#include <metadata/taglibmetareader.h>
#include <metadata/cueparser.h>

namespace xamp::metadata {

using xamp::metadata::TaglibMetadataReader;
using xamp::base::ImbueFileFromBom;

static void skip_spaces(std::wistream & istr) {
	while (istr && (istr.peek() == L' ' || istr.peek() == L'\t')) {
		istr.get();
	}
}

static void skip_line(std::wistream & istr) {
	while (istr && (istr.peek() != L'\n' && istr.peek() != L'\r')) {
		istr.get();
	}
	if (istr) {
		auto c = istr.get();
		if (istr && (c == L'\r' && istr.peek() == L'\n')) {
			istr.get();
		}
	}
}

static std::wstring extract_token(std::wistream & istr) {
	if (!istr) {
		return L"";
	}

	skip_spaces(istr);

	const auto quoted = istr.peek() == '"';
	if (quoted) {
		istr.get();
	}

	std::wstring cmd;

	while (istr) {
		const auto ch = istr.peek();
		if (ch == '\n' || ch == '\r') {
			break;
		}
		if (quoted) {
			if (ch == '"') {
				return cmd;
			}
		}
		else {
			if (ch == ' ' || ch == '\t') {
				return cmd;
			}
		}
		cmd.push_back(istr.get());
	}
	return cmd;
}

static uint32_t parse_time(const std::wstring &str) {
	std::array<uint32_t, 3> values;
	values.fill(0);

	auto itr = values.begin();
	const auto quoted_itr = values.begin() + 2;

	for (const wchar_t i : str) {
		if (i == ':') {
			if (itr != quoted_itr) {
				++itr;
				continue;
			}
			break;
		}
		*itr = *itr * 10 + str[i] - '0';
	}
	return (values[0] * 60 + values[1]) * 75 + values[2];
}

CueParser::CueParser() {
}

bool CueParser::parse_file(const std::wstring & file_path) {
	std::wifstream file;
	file.open(file_path, std::ios::binary);

	if (!file.is_open()) {
		return false;
	}

	file_path_ = Path(file_path).parent_path();

    ImbueFileFromBom(file);
	return parse_stream(file);
}

TrackGroup * CueParser::get_last_track_group() {
	if (tracks_.empty()) {
		return nullptr;
	}
	return &tracks_.back();
}

void CueParser::parse_gap(std::wistream & istr, bool post) {
	const auto gap_str = extract_token(istr);
	const auto gap = parse_time(gap_str);

	const auto track_group = get_last_track_group();
	if (!track_group) {
		return;
	}

	if (post) {
		track_group->postgap = gap;
	}
	else {
		track_group->pregap = gap;
	}
}

bool CueParser::parse_stream(std::wistream & istr) {
	auto parse_file = [this](std::wistream &istr) {
		file_ = extract_token(istr);
	};

	auto parse_catalog = [this](std::wistream &istr) {
		auto track_group = get_last_track_group();
		if (track_group) {
			track_group->meta.catalog = extract_token(istr);
		}
		else {
			meta_.catalog = extract_token(istr);
		}		
	};

	auto parse_performer = [this](std::wistream &istr) {
		auto track_group = get_last_track_group();
		if (track_group) {
			track_group->meta.performer = extract_token(istr);
		}
		else {
			meta_.performer = extract_token(istr);
		}
	};

	auto parse_songwriter = [this](std::wistream &istr) {
		auto track_group = get_last_track_group();
		if (track_group) {
			track_group->meta.songwriter = extract_token(istr);
		}
		else {
			meta_.songwriter = extract_token(istr);
		}
	};

	auto parse_title = [this](std::wistream &istr) {
		auto track_group = get_last_track_group();
		if (track_group) {
			track_group->meta.title = extract_token(istr);
		}
		else {
			meta_.title = extract_token(istr);
		}
	};

	auto parse_track = [this](std::wistream &istr) {
		const auto track_no_str = extract_token(istr);

		std::wistringstream wi(track_no_str);
		auto track_no = 0;
		wi >> track_no;

		TrackGroup tg{};
		tg.track_no = track_no;
		tracks_.push_back(tg);
	};

	auto parse_index = [this](std::wistream &istr) {
		const auto index_no_str = extract_token(istr);

		std::wistringstream wi(index_no_str);
		auto index_no = 0;
		wi >> index_no;

		if (index_no > 1) {
			return;
		}

		const auto time_str = extract_token(istr);
		const auto offset = parse_time(time_str);

		const auto track_group = get_last_track_group();
		if (!track_group) {
			return;
		}

		if (index_no == 0) {
			track_group->index0 = offset;
		}
		else {
			track_group->index1 = offset;
		}
	};

	auto parse_pregap = [this](std::wistream &istr) {
		parse_gap(istr, false);
	};

	auto parse_postgap = [this](std::wistream &istr) {
		parse_gap(istr, true);
	};

	auto parse_isrc = [this](std::wistream &istr) {
		auto track_group = get_last_track_group();
		if (!track_group) {
			return;
		}
		track_group->meta.isrc = extract_token(istr);
	};

	auto parse_date = [this](std::wistream &istr) {
		auto track_group = get_last_track_group();

		if (track_group) {
			track_group->meta.date = extract_token(istr);
		}
		else {
			meta_.date = extract_token(istr);
		}
	};

	auto parse_genre = [this](std::wistream &istr) {
		auto track_group = get_last_track_group();

		if (track_group) {
			track_group->meta.genre = extract_token(istr);
		}
		else {
			meta_.genre = extract_token(istr);
		}
	};

	auto parse_comment = [this](std::wistream &istr) {
		auto track_group = get_last_track_group();

		if (track_group) {
			track_group->meta.comment = extract_token(istr);
		}
		else {
			meta_.comment = extract_token(istr);
		}
	};

	auto parse_compilation = [this](std::wistream &istr) {
		auto track_group = get_last_track_group();

		if (track_group) {
			track_group->meta.compilation = extract_token(istr);
		}
		else {
			meta_.compilation = extract_token(istr);
		}
	};

	const ParseFuncMap rem_cmds{
		{ L"DATE", parse_catalog },
		{ L"GENRE", parse_genre },
		{ L"COMMENT", parse_comment },
		{ L"COMPILATION", parse_compilation },
	};

	auto parse_rem = [this, rem_cmds](std::wistream &istr) {
		parse_cmd(rem_cmds, istr);
	};

	const ParseFuncMap cmds {
		{ L"CATALOG", parse_catalog },
		{ L"REM", parse_rem },
		{ L"TRACK", parse_track },
		{ L"FILE", parse_file },
		{ L"PERFORMER", parse_performer },
		{ L"SONGWRITER", parse_songwriter },
		{ L"TITLE", parse_title },
		{ L"INDEX", parse_index },
		{ L"PREGAP", parse_pregap },
		{ L"POSTGAP", parse_postgap },
		{ L"ISRC", parse_isrc },
	};

	while (istr) {
		parse_cmd(cmds, istr);
		skip_line(istr);
	}

	process_gap();
	sheets_ = process_sheet();
	return true;
}

void CueParser::parse_cmd(const ParseFuncMap &cmds, std::wistream & istr) {
	skip_spaces(istr);

	const auto cmd = extract_token(istr);

	const auto itr = cmds.find(cmd);
	if (itr != cmds.end()) {
		(*itr).second(istr);
	}
}

void CueParser::process_gap() {
	auto last = -1;

	for (auto &track_group : tracks_) {
		if (track_group.index0 == -1 || track_group.index1 == -1) {
			const auto pregap = track_group.pregap != -1 ? track_group.pregap : 0;

			if (track_group.index1 != -1) {
				track_group.index0 = track_group.index1 - pregap;
			}
			else {
				track_group.index1 = track_group.index0 + pregap;
			}
		}

		if (last != -1 && track_group.index0 < last) {
			return;
		}

		const auto postgap = track_group.postgap != -1 ? track_group.postgap : 0;
		last = track_group.index1 + postgap;
	}
}

CueSheet CueParser::process_sheet() {
	CueSheet sheet;
	
	const auto group = get_last_track_group();
	sheet.track_base = group->track_no + 1 - tracks_.size();
	sheet.tracks.resize(tracks_.size());
	sheet.meta = meta_;
	
	std::vector<TrackGroup>::iterator prev;
	auto idx = 0;

	for (auto itr = tracks_.begin(); itr != tracks_.end(); ++itr) {
		sheet.tracks[idx].offset = (itr->index1 / 75.0);
		sheet.tracks[idx].length = -1;

		if (idx > 0) {
			const auto postgap = prev->postgap != -1 ? prev->postgap : 0;
			sheet.tracks[idx - 1].length = ((itr->index0 - prev->index1 - postgap) / 75.0);
		}

		sheet.tracks[idx].meta = itr->meta;
		
		prev = itr;
		++idx;
	}

	// Get last track duration!
	TaglibMetadataReader reader;	
	reader.Extract(file_path_ + L"\\" + file_, metadata_);
	sheet.tracks.back().length = metadata_.duration - sheet.tracks.back().offset;

	return sheet;
}

std::vector<Metadata> CueParser::get_metadatas() {
	std::vector<Metadata> metadatas;

	metadata_.is_cuefile = true;

	auto track = 1;
	for (const auto &sheet : sheets_.tracks) {
		metadata_.track = track++;		
		metadata_.album = sheets_.meta.title;
		metadata_.artist = sheets_.meta.performer;
		metadata_.title = sheet.meta.title;
		metadata_.duration = sheet.length;
		metadata_.offset = sheet.offset;
		metadatas.push_back(metadata_);
	}

	return metadatas;
}

}

#endif