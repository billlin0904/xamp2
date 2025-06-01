//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/pimplptr.h>
#include <base/memory.h>

#include <metadata/imetadatawriter.h>

XAMP_METADATA_NAMESPACE_BEGIN

class TaglibMetadataWriter final : public IMetadataWriter {
public:
    TaglibMetadataWriter();

    XAMP_PIMPL(TaglibMetadataWriter)

    void Open(const Path& path) override;

    void WriteReplayGain(const ReplayGain& replay_gain) override;
   
    void Write(TrackInfo const& track_info) override;

    void WriteTitle(const std::wstring & title) override;

    void WriteArtist(const std::wstring & artist) override;

    void WriteAlbum(const std::wstring & album) override;

    void WriteTrack(uint32_t track) override;

    void WriteComment(const std::wstring& comment) override;

    void WriteGenre(const std::wstring& genre) override;

    void WriteYear(uint32_t year) override;

    void WriteEmbeddedCover(const std::vector<uint8_t> &image) const override;

    void RemoveEmbeddedCover() override;

    [[nodiscard]] bool CanWriteEmbeddedCover() const override;
private:
    class TaglibMetadataWriterImpl;
    ScopedPtr<TaglibMetadataWriterImpl> writer_;
};

XAMP_METADATA_NAMESPACE_END
