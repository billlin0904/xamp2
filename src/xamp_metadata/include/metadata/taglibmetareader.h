//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
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

    void Open(ArchiveEntry archive_entry);

    void Open(const Path& path) override;

    std::expected<ReplayGain, ParseMetadataError> ReadReplayGain() override;
    
    std::expected<TrackInfo, ParseMetadataError> Extract() override;

    std::expected<std::vector<std::byte>, ParseMetadataError> ReadEmbeddedCover() override;

    static HashSet<std::string> const & GetSupportFileExtensions();

    [[nodiscard]] bool IsSupported() const override;
private:
    class TaglibMetadataReaderImpl;
    ScopedPtr<TaglibMetadataReaderImpl> reader_;
};

XAMP_METADATA_NAMESPACE_END

