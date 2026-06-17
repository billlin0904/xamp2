//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/memory.h>
#include <base/memory.h>

#include <metadata/imetadatawriter.h>

XAMP_METADATA_NAMESPACE_BEGIN

class TaglibMetadataWriter final : public IMetadataWriter {
public:
    TaglibMetadataWriter();

    XAMP_PIMPL(TaglibMetadataWriter)

    void open(const Path& path) override;

    void writeReplayGain(const ReplayGain& replay_gain) override;
   
    void write(TrackInfo const& track_info) override;

    void writeTitle(const std::wstring & title) override;

    void writeArtist(const std::wstring & artist) override;

    void writeAlbum(const std::wstring & album) override;

    void writeTrack(uint32_t track) override;

    void writeComment(const std::wstring& comment) override;

    void writeGenre(const std::wstring& genre) override;

    void writeYear(uint32_t year) override;

    void writeEmbeddedCover(const std::vector<uint8_t> &image) const override;

    void removeEmbeddedCover() override;

    [[nodiscard]] bool canWriteEmbeddedCover() const override;
private:
    class TaglibMetadataWriterImpl;
    ScopedPtr<TaglibMetadataWriterImpl> writer_;
};

XAMP_METADATA_NAMESPACE_END
