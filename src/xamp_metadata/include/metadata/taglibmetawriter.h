//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <metadata/imetadatawriter.h>

#include <base/pimplptr.h>
#include <base/align_ptr.h>

XAMP_METADATA_NAMESPACE_BEGIN

class TaglibMetadataWriter final : public IMetadataWriter {
public:
    TaglibMetadataWriter();

	XAMP_PIMPL(TaglibMetadataWriter)

    [[nodiscard]] bool IsFileReadOnly(const Path& path) const override;

    void WriteReplayGain(Path const& path, const ReplayGain& replay_gain) override;
   
    void Write(Path const & path, TrackInfo const& track_info) override;

    void WriteTitle(const Path & path, const std::wstring & title) const;

    void WriteArtist(const Path & path, const std::wstring & artist) const;

    void WriteAlbum(const Path & path, const std::wstring & album) const;

    void WriteTrack(const Path & path, int32_t track) const;

    void WriteEmbeddedCover(const Path & path, const Vector<uint8_t> &image) const override;

    void RemoveEmbeddedCover(const Path& path) override;

    [[nodiscard]] bool CanWriteEmbeddedCover(const Path& path) const override;
private:
    class TaglibMetadataWriterImpl;
    AlignPtr<TaglibMetadataWriterImpl> writer_;
};

XAMP_METADATA_NAMESPACE_END
