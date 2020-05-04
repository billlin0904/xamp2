#include <metadata/metadataextractadapter.h>
#include <metadata/metadatareader.h>

namespace xamp::metadata {

void FromPath(const Path& path, MetadataExtractAdapter* adapter, MetadataReader *reader) {
    using namespace std::filesystem;
    const directory_options options = (
        directory_options::follow_directory_symlink |
        directory_options::skip_permission_denied
        );

    if (is_directory(path)) {
        Path root_path;
        for (const auto& file_or_dir : RecursiveDirectoryIterator(path, options)) {
            if (adapter->IsCancel()) {
                return;
            }

            const auto current_path = file_or_dir.path();
            if (root_path.empty()) {
                root_path = current_path;
            }

            auto parent_path = root_path.parent_path();
            auto cur_path = current_path.parent_path();
            if (parent_path != cur_path) {
                adapter->OnWalkNext();
                root_path = current_path;
            }

            if (!is_directory(current_path)) {
                if (reader->IsSupported(current_path)) {
                    adapter->OnWalk(path, reader->Extract(current_path));
                }
            }
        }
        adapter->OnWalkNext();
    }
    else {
        if (reader->IsSupported(path)) {
            adapter->OnWalk(path, reader->Extract(path));
            adapter->OnWalkNext();
        }
    }
}

}
