#include <base/platform.h>
#include <stream/podcastcache.h>

namespace xamp::stream {

std::string ToCacheID(Path const& file_path) {
    Path path(file_path);
    auto dot_pos = path.filename().string().find(".");
    auto cache_id = path.filename().string().substr(0, dot_pos);
    return cache_id;
}

PadcastFileCache::PadcastFileCache(std::string const& cache_id, std::string const &file_ext)
    : is_completed_(false)
    , cache_id_(cache_id) {
    auto file_name = MakeTempFileName() + file_ext;
    auto tmp_dir_path =	Fs::temp_directory_path();
    tmp_dir_path /= file_name;
    path_ = tmp_dir_path;
    file_.open(path_, std::ios::binary);
}

PadcastFileCache::~PadcastFileCache() {
    try {
        std::filesystem::remove(path_);
    } catch (...) {
    }
}

[[nodiscard]] bool PadcastFileCache::IsOpen() const {
    return file_.is_open();
}

void PadcastFileCache::Write(void const* buf, size_t buf_size) {
    file_.write(static_cast<const char*>(buf), buf_size);
}

void PadcastFileCache::Close() {
    file_.close();
    is_completed_ = true;
}

[[nodiscard]] Path PadcastFileCache::GetFilePath() const {
    return path_;
}

[[nodiscard]] bool PadcastFileCache::IsCompleted() const noexcept {
    return is_completed_;
}

[[nodiscard]] std::string PadcastFileCache::GetCacheID() {
    return cache_id_;
}

std::shared_ptr<PadcastFileCache> PodcastFileCacheManager::GetOrAdd(std::string const& cache_id) {
    auto cache = cache_.Find(cache_id);
    bool found = false;
    if (!cache) {
        cache_.AddOrUpdate(cache_id, std::make_shared<PadcastFileCache>(cache_id, ".m4a"));
        cache = cache_.Find(cache_id);
    } else {
        found = true;
    }
    if (found && !(*cache)->IsCompleted()) {
        cache_.Erase(cache_id);
        cache_.AddOrUpdate(cache_id, std::make_shared<PadcastFileCache>(cache_id, ".m4a"));
        cache = cache_.Find(cache_id);
    }
    return (*cache)->shared_from_this();
}

void PodcastFileCacheManager::Remove(std::string const& cache_id) {
    cache_.Erase(cache_id);
}

void PodcastFileCacheManager::Remove(std::shared_ptr<PadcastFileCache> const & file_cache) {
    Remove(file_cache->GetCacheID());
}

void PodcastFileCacheManager::Load(Path const& path) {
    for (auto const& file_or_dir
            : RecursiveDirectoryIterator(Fs::absolute(path), kIteratorOptions)) {
        auto cache_id = ToCacheID(file_or_dir);
        cache_.AddOrUpdate(cache_id, std::make_shared<PadcastFileCache>(cache_id, ".m4a"));
    }
}

}
