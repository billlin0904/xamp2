//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/metadata.h>
#include <base/rng.h>
#include <base/shared_singleton.h>
#include <base/logger.h>
#include <base/exception.h>
#include <base/ithreadpool.h>
#include <base/stopwatch.h>
#include <base/lrucache.h>
#include <base/stl.h>
#include <base/dsdsampleformat.h>
#include <base/vmmemlock.h>

#include <stream/filestream.h>

#include <output_device/deviceinfo.h>

#include <player/playstate.h>

using namespace xamp::base;
using namespace xamp::stream;
using namespace xamp::player;
using namespace xamp::output_device;

