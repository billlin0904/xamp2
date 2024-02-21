//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
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

    [[nodiscard]] bool IsFileReadOnly(const Path& path) const override;

    void WriteReplayGain(Path const& path, const ReplayGain& replay_gain) override;
   
    void Write(Path const & path, TrackInfo const& track_info) override;

    void WriteTitle(const Path & path, const std::wstring & title) override;

    void WriteArtist(const Path & path, const std::wstring & artist) override;

    void WriteAlbum(const Path & path, const std::wstring & album) override;

    void WriteTrack(const Path & path, uint32_t track) override;

    void WriteComment(const Path& path, const std::wstring& comment) override;

    void WriteGenre(const Path& path, const std::wstring& genre) override;

    void WriteYear(const Path& path, uint32_t year) override;

    void WriteEmbeddedCover(const Path & path, const Vector<uint8_t> &image) const override;

    void RemoveEmbeddedCover(const Path& path) override;

    [[nodiscard]] bool CanWriteEmbeddedCover(const Path& path) const override;
private:
    class TaglibMetadataWriterImpl;
    AlignPtr<TaglibMetadataWriterImpl> writer_;
};

XAMP_METADATA_NAMESPACE_END
