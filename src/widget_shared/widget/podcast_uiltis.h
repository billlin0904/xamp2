//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>
#include <vector>
#include <base/stl.h>
#include <base/trackinfo.h>

#include <QList>
#include <QMetaType>

#include <widget/str_utilts.h>
#include <widget/widget_shared_global.h>

using namespace xamp::base;

struct XAMP_WIDGET_SHARED_EXPORT MbDiscIdTrack {
	int32_t track{ 0 };
	std::wstring title;
};

struct XAMP_WIDGET_SHARED_EXPORT MbDiscIdInfo {
	std::string disc_id;
	std::wstring album;
	std::wstring artist;
	
	ForwardList<MbDiscIdTrack> tracks;
};

Q_DECLARE_METATYPE(MbDiscIdInfo)

Vector<TrackInfo> ParseJson(QString const& json);

std::pair<std::string, MbDiscIdInfo> ParseMbDiscIdXml(QString const& src);

QString ParseCoverUrl(QString const& src);
