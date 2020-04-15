#include <base/logger.h>
#include <metadata/metadatareader.h>

namespace xamp::metadata {

void FromPath(const Path& path, MetadataExtractAdapter* adapter, MetadataReader *reader) {
    if (is_directory(path)) {
        Path root_path;
        for (RecursiveDirectoryIterator itr(path), end; itr != end && !adapter->IsCancel(); ++itr) {
            const auto current_path = (*itr).path();
            if (root_path.empty()) {
                root_path = current_path;
            }

            if (!is_directory(current_path)) {
                if (reader->IsSupported(current_path)) {
                    adapter->OnWalk(path, std::move(reader->Extract(current_path)));
                }
            }

            auto parent_path = root_path.parent_path();
            auto cur_path = current_path.parent_path();
            if (parent_path != cur_path) {                
                adapter->OnWalkNext();
                root_path = current_path;
            }
        }
        adapter->OnWalkNext();
    }
    else {
        if (reader->IsSupported(path)) {
            adapter->OnWalk(path, std::move(reader->Extract(path)));
            adapter->OnWalkNext();
        }
    }
}

}
