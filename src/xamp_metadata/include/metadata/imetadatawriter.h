//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/fs.h>
#include <base/stl.h>
#include <base/trackinfo.h>
#include <metadata/metadata.h>

XAMP_METADATA_NAMESPACE_BEGIN

class XAMP_METADATA_API XAMP_NO_VTABLE IMetadataWriter {
public:
    XAMP_BASE_CLASS(IMetadataWriter)

    virtual void open(const Path& path) = 0;

    virtual void write(const TrackInfo& track_info) = 0;

    virtual void writeArtist(const std::wstring& artist) = 0;

    virtual void writeAlbum(const std::wstring& album) = 0;

    virtual void writeTitle(const std::wstring& title) = 0;
    
    virtual void writeTrack(uint32_t track) = 0;

    virtual void writeGenre(const std::wstring& genre) = 0;

    virtual void writeComment(const std::wstring& comment) = 0;

    virtual void writeYear(uint32_t year) = 0;

    virtual void writeReplayGain(const ReplayGain& replay_gain) = 0;

    virtual void writeEmbeddedCover(const std::vector<uint8_t> & image) const = 0;

    virtual void removeEmbeddedCover() = 0;

    [[nodiscard]] virtual bool canWriteEmbeddedCover() const = 0;
protected:
    IMetadataWriter() = default;
};

XAMP_METADATA_NAMESPACE_END
