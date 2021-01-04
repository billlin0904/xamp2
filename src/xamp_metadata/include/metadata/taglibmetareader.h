//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/stl.h>
#include <base/align_ptr.h>

#include <metadata/metadata.h>
#include <metadata/metadatareader.h>

namespace xamp::metadata {

class XAMP_METADATA_API TaglibMetadataReader final : public MetadataReader {
public:
    TaglibMetadataReader();

    XAMP_PIMPL(TaglibMetadataReader)
    
    Metadata Extract(Path const &path) override;

    const std::vector<uint8_t>& ExtractEmbeddedCover(Path const &path) override;

    [[nodiscard]] HashSet<std::string> const & GetSupportFileExtensions() const override;

    [[nodiscard]] bool IsSupported(Path const & path) const override;
private:
    class TaglibMetadataReaderImpl;
	AlignPtr<TaglibMetadataReaderImpl> reader_;
};

}

