//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <metadata/imetadatareader.h>

#include <base/stl.h>
#include <base/memory.h>
#include <base/archivefile.h>

XAMP_METADATA_NAMESPACE_BEGIN

class XAMP_METADATA_API TaglibMetadataReader final : public IMetadataReader {
public:
    TaglibMetadataReader();

    XAMP_PIMPL(TaglibMetadataReader)

    void open(ArchiveEntry archive_entry) override;

    void open(const Path& path) override;

    std::expected<ReplayGain, ParseMetadataError> readReplayGain() override;
    
    std::expected<TrackInfo, ParseMetadataError> extract() override;

    std::expected<std::vector<std::byte>, ParseMetadataError> readEmbeddedCover() override;

    static HashSet<std::string> const & getSupportFileExtensions();

    [[nodiscard]] bool isSupported() const override;
private:
    class TaglibMetadataReaderImpl;
    ScopedPtr<TaglibMetadataReaderImpl> reader_;
};

XAMP_METADATA_NAMESPACE_END

