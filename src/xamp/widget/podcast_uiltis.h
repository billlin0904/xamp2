//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>
#include <vector>
#include <base/stl.h>
#include <base/metadata.h>

using namespace xamp::base;

Vector<Metadata> parseJson(QString const& json);

std::pair<std::string, ForwardList<Metadata>> parsePodcastXML(QString const& src);
