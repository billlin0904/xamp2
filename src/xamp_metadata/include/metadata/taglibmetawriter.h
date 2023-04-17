//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
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
   
    void Write(Path const & path, TrackInfo const& trackinfo) override;

    void WriteTitle(Path const & path, std::wstring const & title) const;

    void WriteArtist(Path const & path, std::wstring const & artist) const;

    void WriteAlbum(Path const & path, std::wstring const & album) const;

    void WriteTrack(Path const & path, int32_t track) const;

    void WriteEmbeddedCover(Path const & path, std::vector<uint8_t> const &image) const override;
private:
    class TaglibMetadataWriterImpl;
    PimplPtr<TaglibMetadataWriterImpl> writer_;
};

XAMP_METADATA_NAMESPACE_END
