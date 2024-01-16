//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <metadata/imetadatareader.h>

#include <base/stl.h>
#include <base/align_ptr.h>
#include <base/pimplptr.h>

XAMP_METADATA_NAMESPACE_BEGIN

class XAMP_METADATA_API TaglibMetadataReader final : public IMetadataReader {
public:
    TaglibMetadataReader();

    XAMP_PIMPL(TaglibMetadataReader)

	std::optional<ReplayGain> GetReplayGain(const Path& path) override;
    
    TrackInfo Extract(Path const &path) override;

    const Vector<uint8_t>& ReadEmbeddedCover(Path const &path) override;

    static [[nodiscard]] HashSet<std::string> const & GetSupportFileExtensions();

    [[nodiscard]] bool IsSupported(Path const & path) const override;
private:
    class TaglibMetadataReaderImpl;
    AlignPtr<TaglibMetadataReaderImpl> reader_;
};

XAMP_METADATA_NAMESPACE_END

