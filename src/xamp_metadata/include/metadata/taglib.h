//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <metadata/metadata.h>

#include <tlist.h>
#include <tfilestream.h>
#include <fileref.h>
#include <tag.h>
#include <tdebuglistener.h>

#include <flac/flacfile.h>
#include <flac/flacpicture.h>
#include <flac/flacproperties.h>
#include <ogg/xiphcomment.h>

#include <dsdiff/dsdifffile.h>
#include <dsf/dsffile.h>

#include <mp4file.h>

#include <ape/apetag.h>
#include <ape/apefile.h>

#include <mpegfile.h>
#include <id3v2/frames/textidentificationframe.h>
#include <id3v2/frames/attachedpictureframe.h>
#include <id3v2/id3v2tag.h>
#include <id3v2/id3v2framefactory.h>
#include <id3v2/id3v2header.h>
#include <id3v2/id3v2extendedheader.h>

XAMP_METADATA_NAMESPACE_BEGIN

using namespace TagLib;

XAMP_METADATA_NAMESPACE_END

