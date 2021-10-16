//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/memory.h>
#include <base/align_ptr.h>

#include <metadata/metadata.h>
#include <metadata/imetadatawriter.h>

namespace xamp::metadata {

class TaglibMetadataWriter final : public IMetadataWriter {
public:
    TaglibMetadataWriter();

	XAMP_PIMPL(TaglibMetadataWriter)

    [[nodiscard]] bool IsFileReadOnly(const Path& path) const override;
   
    void Write(Path const & path, Metadata const& metadata) override;

    void WriteTitle(Path const & path, std::wstring const & title) const;

    void WriteArtist(Path const & path, std::wstring const & artist) const;

    void WriteAlbum(Path const & path, std::wstring const & album) const;

    void WriteTrack(Path const & path, int32_t track) const;

    void WriteEmbeddedCover(Path const & path, std::vector<uint8_t> const &image) const override;
private:
    class TaglibMetadataWriterImpl;
	AlignPtr<TaglibMetadataWriterImpl> writer_;
};

}
