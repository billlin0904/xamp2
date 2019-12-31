//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <unordered_set>

#include <base/memory.h>
#include <base/align_ptr.h>

#include <metadata/metadata.h>
#include <metadata/metadatareader.h>

namespace xamp::metadata {

class XAMP_METADATA_API TaglibMetadataReader final : public MetadataReader {
public:
    TaglibMetadataReader();

    XAMP_PIMPL(TaglibMetadataReader)
    
    Metadata Extract(const Path &path) override;

    void ExtractFromPath(const Path &path, MetadataExtractAdapter* adapter) override;

    const std::vector<uint8_t>& ExtractEmbeddedCover(const Path &path) override;

    const std::unordered_set<std::string> & GetSupportFileExtensions() const override;

    bool IsSupported(const Path & path) const override;
private:
    class TaglibMetadataReaderImpl;
	AlignPtr<TaglibMetadataReaderImpl> reader_;
};

}

