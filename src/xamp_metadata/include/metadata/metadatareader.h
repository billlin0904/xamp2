//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <vector>

#include <base/alignstl.h>
#include <base/metadata.h>

#include <metadata/metadata.h>
#include <metadata/metadataextractadapter.h>

namespace xamp::metadata {

class XAMP_METADATA_API XAMP_NO_VTABLE MetadataReader {
public:
    virtual Metadata Extract(const Path &path) = 0;
 
    virtual const std::vector<uint8_t>& ExtractEmbeddedCover(const Path &path) = 0;

    virtual const Set<std::string> & GetSupportFileExtensions() const = 0;

    virtual bool IsSupported(const Path & path) const = 0;
protected:
    MetadataReader() = default;
};

XAMP_METADATA_API inline void FromPath(const Path& path, MetadataExtractAdapter* adapter, MetadataReader *reader) {
    if (is_directory(path)) {
        Path root_path;
        for (RecursiveDirectoryIterator itr(path), end; itr != end && !adapter->IsCancel(); ++itr) {
            const auto current_path = (*itr).path();
            if (root_path.empty()) {
                root_path = current_path;
            }            

            if (!is_directory(current_path)) {
                if (reader->IsSupported(current_path)) {
                    adapter->OnWalk(path, reader->Extract(current_path));
                }
            }

            if (root_path.parent_path() != current_path.parent_path()) {
                adapter->OnWalkNext();
                root_path = current_path;
            }
        }
        adapter->OnWalkNext();
    }
    else {
        adapter->OnWalk(path, reader->Extract(path));
        adapter->OnWalkNext();
    }
}

}
