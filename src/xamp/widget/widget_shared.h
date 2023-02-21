//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/trackinfo.h>
#include <base/rng.h>
#include <base/shared_singleton.h>
#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/exception.h>
#include <base/ithreadpoolexecutor.h>
#include <base/stopwatch.h>
#include <base/lrucache.h>
#include <base/stl.h>
#include <base/executor.h>
#include <base/dsdsampleformat.h>
#include <base/vmmemlock.h>
#include <base/str_utilts.h>
#include <base/align_ptr.h>
#include <base/lrucache.h>
#include <base/rcu_ptr.h>

#include <metadata/api.h>
#include <stream/api.h>
#include <output_device/api.h>
#include <player/api.h>

#include <output_device/deviceinfo.h>

#include <metadata/imetadatawriter.h>
#include <metadata/imetadatareader.h>

#include <player/playstate.h>

using namespace xamp::base;
using namespace xamp::metadata;
using namespace xamp::stream;
using namespace xamp::player;
using namespace xamp::output_device;

#define MAX_SANDBOX_MODE
