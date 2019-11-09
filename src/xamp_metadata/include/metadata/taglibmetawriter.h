//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/memory.h>
#include <base/align_ptr.h>

#include <metadata/metadata.h>
#include <metadata/metadatawriter.h>

namespace xamp::metadata {

class XAMP_METADATA_API TaglibMetadataWriter final : public MetadataWriter {
public:
    TaglibMetadataWriter();

	XAMP_PIMPL(TaglibMetadataWriter)

	bool IsFileReadOnly(const Path& path) const override;
   
    void Write(const Path& path, Metadata& metadata) override;

    void WriteTitle(const Path& path, const std::wstring& title) const;

    void WriteArtist(const Path& path, const std::wstring& artist) const;

    void WriteAlbum(const Path& path, const std::wstring& album) const;

    void WriteTrack(const Path& path, int32_t track) const;

	void WriteEmbeddedCover(const Path& path, const std::vector<uint8_t> &image) const;
private:
    class TaglibMetadataWriterImpl;
	AlignPtr<TaglibMetadataWriterImpl> writer_;
};

}
