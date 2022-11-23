#include <metadata/imetadatareader.h>
#include <metadata/imetadatawriter.h>

#include <metadata/imetadataextractadapter.h>
#include <metadata/taglibmetareader.h>
#include <metadata/taglibmetawriter.h>

#include <metadata/api.h>

namespace xamp::metadata {

AlignPtr<IMetadataReader> MakeMetadataReader() {
	return MakeAlign<IMetadataReader, TaglibMetadataReader>();
}

AlignPtr<IMetadataWriter> MakeMetadataWriter() {
	return MakeAlign<IMetadataWriter, TaglibMetadataWriter>();
}

template <typename TDirectoryIterator>
static void ScanFolderImpl(Path const& path,
    const std::function<bool(const Path&)> &is_accept,
    const std::function<void(const Path &)> &walk,
    const std::function<void(const Path&, bool)> &end_walk) {
    if (Fs::is_directory(path)) {
        Path root_path;

        for (auto const& file_or_dir : TDirectoryIterator(Fs::absolute(path), kIteratorOptions)) {
            auto const& current_path = file_or_dir.path();
            if (root_path.empty()) {
                root_path = current_path;
            }

            auto parent_path = root_path.parent_path();
            auto cur_path = current_path.parent_path();
            if (parent_path != cur_path) {
                end_walk(DirectoryEntry(parent_path), true);
                root_path = current_path;
            }

            if (is_accept(current_path)) {
                walk(current_path);
            }
        }

        end_walk(DirectoryEntry(root_path.parent_path()), false);
    }
    else {
        if (is_accept(path)) {
            walk(path);
            end_walk(DirectoryEntry(path.parent_path()), false);
        }
    }
}

void ScanFolder(Path const& path, IMetadataExtractAdapter* adapter, bool is_recursive) {
    const auto is_accept = [adapter](auto path) {
        return adapter->IsAccept(path);
    };

    const auto walk = [adapter](auto path) {
        adapter->OnWalk(path);
    };

    const auto walk_end = [adapter](auto path, bool is_new) {
        adapter->OnWalkEnd(DirectoryEntry(path));
        if (is_new) {
            adapter->OnWalkNew();
        }
    };

    if (!is_recursive) {
        ScanFolderImpl<DirectoryIterator>(path, is_accept, walk, walk_end);
    } else {
        ScanFolderImpl<RecursiveDirectoryIterator>(path, is_accept, walk, walk_end);
    }
}

void ScanFolder(Path const& path,
    const std::function<bool(const Path&)>& is_accept,
    const std::function<void(const Path&)>& walk,
    const std::function<void(const Path&, bool)>& end_walk,
    bool is_recursive) {
    if (!is_recursive) {
        ScanFolderImpl<DirectoryIterator>(path, is_accept, walk, end_walk);
    }
    else {
        ScanFolderImpl<RecursiveDirectoryIterator>(path, is_accept, walk, end_walk);
    }
}

}
