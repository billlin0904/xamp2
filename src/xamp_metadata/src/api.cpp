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
static void ScanFolderImpl(Path const& path, IMetadataExtractAdapter* adapter, IMetadataReader* reader) {
    adapter->OnWalkNew();

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
                adapter->OnWalkEnd(DirectoryEntry(parent_path));
                adapter->OnWalkNew();
                root_path = current_path;
            }

            if (!Fs::is_directory(current_path)) {
                if (adapter->IsAccept(current_path)) {
                    adapter->OnWalk(path, reader->Extract(current_path));
                }
            }
        }

        adapter->OnWalkEnd(DirectoryEntry(path));
    }
    else {
        if (adapter->IsAccept(path)) {
            adapter->OnWalk(path, reader->Extract(path));
            adapter->OnWalkEnd(DirectoryEntry(path));
        }
    }
}

void ScanFolder(Path const& path, IMetadataExtractAdapter* adapter, IMetadataReader* reader, bool is_recursive) {
    if (!is_recursive) {
        ScanFolderImpl<DirectoryIterator>(path, adapter, reader);
    } else {
        ScanFolderImpl<RecursiveDirectoryIterator>(path, adapter, reader);
    }
}

}
