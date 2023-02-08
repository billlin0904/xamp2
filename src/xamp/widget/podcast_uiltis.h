//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>
#include <vector>
#include <base/stl.h>
#include <base/trackinfo.h>

#include <widget/str_utilts.h>

using namespace xamp::base;

struct MbDiscIdTrack {
	int32_t track{ 0 };
	std::wstring title;
};

struct MbDiscIdInfo {
	std::string disc_id;
	std::wstring album;
	std::wstring artist;
	
	ForwardList<MbDiscIdTrack> tracks;
};

Q_DECLARE_METATYPE(MbDiscIdInfo)

ForwardList<TrackInfo> ParseJson(QString const& json);

std::pair<std::string, MbDiscIdInfo> ParseMbDiscIdXml(QString const& src);

QString ParseCoverUrl(QString const& src);
