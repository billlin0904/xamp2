//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>
#include <vector>
#include <base/stl.h>
#include <base/metadata.h>
#include <widget/str_utilts.h>

using namespace xamp::base;

Vector<Metadata> parseJson(QString const& json);

std::pair<std::string, ForwardList<Metadata>> parsePodcastXML(QString const& src);

std::pair<std::string, ForwardList<Metadata>> parseMbDiscIdXML(QString const& src);

QString parseCoverUrl(QString const& src);
