//=====================================================================================================================
// Copyright (c) 2018 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#if 0

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <chrono>

#include <base/base.h>
#include <base/metadata.h>

namespace xamp::metadata {

using ParseFuncMap = std::unordered_map<std::wstring, std::function<void(std::wistream &)>>;

struct CueMeta {
	std::wstring catalog;
	std::wstring isrc;
	std::wstring performer;
	std::wstring songwriter;
	std::wstring title;
	std::wstring genre;
	std::wstring date;
	std::wstring comment;
	std::wstring compilation;
};

struct Track {
	double offset;
	double length;
	CueMeta meta;
};

struct CueSheet {
	std::wstring file;
	std::vector<Track> tracks;
	int32_t track_base{};
	CueMeta meta;
};

struct TrackGroup {
	uint32_t track_no{ 0 };
	int32_t pregap{ -1 };
	int32_t postgap{ -1 };
	int32_t index0{ -1 };
	int32_t index1{ -1 };
	CueMeta meta;
};

class XAMP_METADATA_API CueParser {
public:
	CueParser();

	XAMP_DISABLE_COPY(CueParser)

	bool parse_file(const std::wstring &file_path);

	bool parse_stream(std::wistream &istr);

	std::vector<Metadata> get_metadatas();

private:
	TrackGroup * get_last_track_group();

	void parse_gap(std::wistream & istr, bool post);

	void parse_cmd(const ParseFuncMap &cmds, std::wistream & istr);

	void process_gap();

	CueSheet process_sheet();
	
	CueMeta meta_;
	CueSheet sheets_;
	std::wstring file_;
	std::wstring file_path_;
	Metadata metadata_;
	std::vector<TrackGroup> tracks_;
};

}

#endif
