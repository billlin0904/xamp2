//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>
#include <vector>
#include <forward_list>
#include <base/metadata.h>

using namespace rapidxml;
using namespace xamp::base;

std::vector<Metadata> parseJson(QString const& json);

std::pair<std::string, std::forward_list<Metadata>> parsePodcastXML(QString const& src);
