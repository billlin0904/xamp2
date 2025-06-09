#include <metadata/taglibmetareader.h>
#include <metadata/taglibiostream.h>
#include <metadata/taglib.h>

#include <base/stl.h>
#include <base/str_utilts.h>
#include <base/memory.h>
#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/singleton.h>
#include <base/exception.h>
#include <base/fastmutex.h>

#include <vector>
#include <sstream>
#include <functional>
#include <regex>

XAMP_METADATA_NAMESPACE_BEGIN
namespace {
    double ParseStringList(const std::string& s, bool string_dummy = true) {
        std::stringstream ss;
        ss << s;
        if (string_dummy) {
            std::string dummy;
            ss >> dummy;
        }
        double d = 0;
        ss >> d;
        return d;
    }

    std::optional<std::vector<std::byte>> GetID3V2TagCover(const ID3v2::Tag* tag) {
        if (!tag) {
            return std::nullopt;
        }
        auto const& frame_list = tag->frameList("APIC");
        if (frame_list.isEmpty()) {
            return std::nullopt;
        }

        const auto* frame = dynamic_cast<ID3v2::AttachedPictureFrame*>(frame_list.front());
        if (!frame) {
            return std::nullopt;
        }
        std::vector<std::byte> buffer;
        buffer.resize(frame->picture().size());
        MemoryCopy(buffer.data(), frame->picture().data(), static_cast<int32_t>(frame->picture().size()));
        return MakeOptional<std::vector<std::byte>>(std::move(buffer));
    }

    std::optional<std::vector<std::byte>> GetApeTagCover(const APE::Tag* tag) {
        auto const& list_map = tag->itemListMap();

        if (!list_map.contains("COVER ART (FRONT)")) {
            return std::nullopt;
        }
        std::vector<std::byte> buffer;
        const ByteVector null_string_terminator(1, 0);
        auto item = list_map["COVER ART (FRONT)"].binaryData();
        auto pos = item.find(null_string_terminator);	// Skip the filename
        if (++pos > 0) {
            auto pic = item.mid(pos);
            buffer.resize(pic.size());
            MemoryCopy(buffer.data(), pic.data(), static_cast<int32_t>(pic.size()));
            return MakeOptional<std::vector<std::byte>>(std::move(buffer));
        }
        return std::nullopt;
    }

    struct XAMP_NO_VTABLE IFileTagReader {
        virtual ~IFileTagReader() = default;
        virtual std::expected<ReplayGain, ParseMetadataError> ReadReplayGain(File* file) = 0;
        virtual std::expected<std::vector<std::byte>, ParseMetadataError> ReadEmbeddedCover(File* file) = 0;
    };

    struct Mp3TagReader : public IFileTagReader {
        std::expected<ReplayGain, ParseMetadataError> ReadReplayGain(File* file) override {
            ReplayGain replay_gain;
            bool found = false;
            if (auto* mp3_file = dynamic_cast<TagLib::MPEG::File*>(file)) {
                if (const auto* tag = mp3_file->ID3v2Tag(false)) {
                    const auto& frame_list = tag->frameList("TXXX");
                    for (auto* it : frame_list) {
                        auto* fr = dynamic_cast<TagLib::ID3v2::UserTextIdentificationFrame*>(it);
                        if (fr) {
                            const auto desc = fr->description().upper();
                            const auto value = ParseStringList(fr->fieldList().toString().to8Bit());
                            if (desc == kReplaygainAlbumGain) {
                                replay_gain.album_gain = value;
                                found = true;
                            }
                            else if (desc == kReplaygainTrackGain) {
                                replay_gain.track_gain = value;
                                found = true;
                            }
                            else if (desc == kReplaygainAlbumPeak) {
                                replay_gain.album_peak = value;
                                found = true;
                            }
                            else if (desc == kReplaygainTrackPeak) {
                                replay_gain.track_peak = value;
                                found = true;
                            }
                            else if (desc == kReplaygainReferenceLoudness) {
                                replay_gain.ref_loudness = value;
                                found = true;
                            }
                        }
                    }
                }
            }
            if (!found) {
                return std::unexpected(ParseMetadataError::PARSE_ERROR_NOT_FOUND);
            }
            return replay_gain;
        }

        std::expected<std::vector<std::byte>, ParseMetadataError> ReadEmbeddedCover(File* file) override {
            if (auto* mpeg_file = dynamic_cast<TagLib::MPEG::File*>(file)) {
                std::optional<std::vector<std::byte>> buffer;
                if (mpeg_file->ID3v2Tag()) {
                    buffer = GetID3V2TagCover(mpeg_file->ID3v2Tag());
                }
                if (!buffer && mpeg_file->APETag()) {
                    buffer = GetApeTagCover(mpeg_file->APETag());
                }
                if (buffer) {
                    return buffer.value();
                }
            }
            return std::unexpected(ParseMetadataError::PARSE_ERROR_NOT_FOUND);
        }
    };

    struct DsfTagReader : public IFileTagReader {
        std::expected<std::vector<std::byte>, ParseMetadataError> ReadEmbeddedCover(File* file) override {
            if (const auto* dsd_file = dynamic_cast<TagLib::DSF::File*>(file)) {
                if (dsd_file->tag()) {
                    auto cover = GetID3V2TagCover(dsd_file->tag());
                    if (cover) {
                        return cover.value();
                    }
                }
            }
            return std::unexpected(ParseMetadataError::PARSE_ERROR_NOT_FOUND);
        }

        std::expected<ReplayGain, ParseMetadataError> ReadReplayGain(File* file) override {
            return std::unexpected(ParseMetadataError::PARSE_ERROR_NOT_FOUND);
        }
    };

    struct DiffTagReader : public IFileTagReader {
        std::expected<std::vector<std::byte>, ParseMetadataError> ReadEmbeddedCover(File* file) override {
            if (const auto* dsd_file = dynamic_cast<TagLib::DSDIFF::File*>(file)) {
                if (dsd_file->ID3v2Tag()) {
                    auto cover = GetID3V2TagCover(dsd_file->ID3v2Tag());
                    if (cover) {
                        return cover.value();
                    }
                }
            }
            return std::unexpected(ParseMetadataError::PARSE_ERROR_NOT_FOUND);
        }

        std::expected<ReplayGain, ParseMetadataError> ReadReplayGain(File* file) override {
            return std::unexpected(ParseMetadataError::PARSE_ERROR_NOT_FOUND);
        }
    };

    struct Mp4TagReader : public IFileTagReader {
        std::expected<std::vector<std::byte>, ParseMetadataError> ReadEmbeddedCover(File* file) override {
            std::vector<std::byte> buffer;

            if (const auto* mp4_file = dynamic_cast<TagLib::MP4::File*>(file)) {
                auto* tag = mp4_file->tag();
                if (!tag) {
                    return std::unexpected(ParseMetadataError::PARSE_ERROR_NOT_FOUND);
                }

                if (!tag->itemMap().contains("covr")) {
                    return std::unexpected(ParseMetadataError::PARSE_ERROR_NOT_FOUND);
                }

                auto cover_list = tag->itemMap()["covr"].toCoverArtList();
                if (cover_list.isEmpty()) {
                    return std::unexpected(ParseMetadataError::PARSE_ERROR_NOT_FOUND);
                }
                if (cover_list[0].data().size() > 0) {
                    buffer.resize(cover_list[0].data().size());
                    MemoryCopy(buffer.data(), cover_list[0].data().data(),
                        static_cast<int32_t>(cover_list[0].data().size()));
                    return buffer;
                }
            }
            return std::unexpected(ParseMetadataError::PARSE_ERROR_NOT_FOUND);
        }

        std::expected<ReplayGain, ParseMetadataError> ReadReplayGain(File* file) override {
            ReplayGain replay_gain;
            auto found = false;
            if (const auto* mp4_file = dynamic_cast<TagLib::MP4::File*>(file)) {
                if (const auto* tag = mp4_file->tag()) {
                    auto const& dict = tag->itemMap();
                    const auto track_gain = kITunesReplaygainTrackGain;
                    if (dict.contains(track_gain)) {
                        replay_gain.track_gain = ParseStringList(dict[track_gain].toStringList()[0].to8Bit(), false);
                        found = true;
                    }
                    const auto track_peak = kITunesReplaygainTrackPeak;
                    if (dict.contains(track_peak)) {
                        replay_gain.track_peak = ParseStringList(dict[track_peak].toStringList()[0].to8Bit(), false);
                        found = true;
                    }
                    const auto album_gain = kITunesReplaygainAlbumGain;
                    if (dict.contains(album_gain)) {
                        replay_gain.album_gain = ParseStringList(dict[album_gain].toStringList()[0].to8Bit(), false);
                        found = true;
                    }
                    const auto album_peak = kITunesReplaygainAlbumPeak;
                    if (dict.contains(String::AsStdString(album_peak))) {
                        replay_gain.album_peak = ParseStringList(dict[album_peak].toStringList()[0].to8Bit(), false);
                        found = true;
                    }
                    const auto reference_loudness = kITunesReplaygainReferenceLoudness;
                    if (dict.contains(reference_loudness)) {
                        replay_gain.album_peak = ParseStringList(dict[reference_loudness].toStringList()[0].to8Bit(), false);
                        found = true;
                    }
                }
            }
            if (!found) {
                return std::unexpected(ParseMetadataError::PARSE_ERROR_NOT_FOUND);
            }
            return replay_gain;
        }

    };

    struct FlacTagReader : public IFileTagReader {
        std::expected<std::vector<std::byte>, ParseMetadataError> ReadEmbeddedCover(File* file) override {
            if (auto* flac_file = dynamic_cast<TagLib::FLAC::File*>(file)) {
                const auto picture_list = flac_file->pictureList();
                if (picture_list.isEmpty()) {
                    return std::unexpected(ParseMetadataError::PARSE_ERROR_NOT_FOUND);
                }
                std::vector<std::byte> buffer;
                for (const auto& picture : picture_list) {
                    if (picture->type() == TagLib::FLAC::Picture::FrontCover) {
                        buffer.resize(picture->data().size());
                        MemoryCopy(buffer.data(), picture->data().data(), picture->data().size());
                        return buffer;
                    }
                }
            }
            return std::unexpected(ParseMetadataError::PARSE_ERROR_NOT_FOUND);
        }

        std::expected<ReplayGain, ParseMetadataError> ReadReplayGain(File* file) override {
            ReplayGain replay_gain;
            bool found = false;
            if (auto* const flac_file = dynamic_cast<TagLib::FLAC::File*>(file)) {
                if (const auto* xiph_comment = flac_file->xiphComment()) {
                    auto field_map = xiph_comment->fieldListMap();
                    for (auto& field : field_map) {
                        if (field.first == kReplaygainAlbumGain) {
                            replay_gain.album_gain = std::stod(field.second[0].to8Bit());
                            found = true;
                        }
                        else if (field.first == kReplaygainTrackPeak) {
                            replay_gain.track_peak = std::stod(field.second[0].to8Bit());
                            found = true;
                        }
                        else if (field.first == kReplaygainAlbumPeak) {
                            replay_gain.album_peak = std::stod(field.second[0].to8Bit());
                            found = true;
                        }
                        else if (field.first == kReplaygainTrackGain) {
                            replay_gain.track_gain = std::stod(field.second[0].to8Bit());
                            found = true;
                        }
                        else if (field.first == kReplaygainReferenceLoudness) {
                            replay_gain.ref_loudness = std::stod(field.second[0].to8Bit());
                            found = true;
                        }
                    }
                }
            }
            if (!found) {
                return std::unexpected(ParseMetadataError::PARSE_ERROR_NOT_FOUND);
            }
            return replay_gain;
        }
    };

    struct OpusTagReader : public IFileTagReader {
        std::expected<std::vector<std::byte>, ParseMetadataError> ReadEmbeddedCover(File* file) override {
            auto* opus_file = dynamic_cast<TagLib::Ogg::Opus::File*>(file);
            if (!opus_file || !opus_file->isValid()) {
                return std::unexpected(ParseMetadataError::PARSE_ERROR_NOT_FOUND);
            }

            auto* comment = opus_file->tag();
            if (!comment) {
                return std::unexpected(ParseMetadataError::PARSE_ERROR_NOT_FOUND);
            }

            auto picture_list = comment->pictureList();
            if (picture_list.isEmpty()) {
                return std::unexpected(ParseMetadataError::PARSE_ERROR_NOT_FOUND);
            }

            std::vector<std::byte> buffer;
            for (const auto& picture : picture_list) {
                if (picture->type() == TagLib::FLAC::Picture::FrontCover) {
                    buffer.resize(picture->data().size());
                    MemoryCopy(buffer.data(), picture->data().data(), picture->data().size());
                    return buffer;
                }
            }
            return std::unexpected(ParseMetadataError::PARSE_ERROR_NOT_FOUND);
        }

        std::expected<ReplayGain, ParseMetadataError> ReadReplayGain(File* file) override {
            auto* opus_file = dynamic_cast<TagLib::Ogg::Opus::File*>(file);
            if (!opus_file || !opus_file->isValid()) {
                return std::unexpected(ParseMetadataError::PARSE_ERROR_NOT_FOUND);
            }

            auto* xiph_comment = opus_file->tag();
            if (!xiph_comment) {
                return std::unexpected(ParseMetadataError::PARSE_ERROR_NOT_FOUND);
            }

            auto found = false;
            auto field_map = xiph_comment->fieldListMap();
            ReplayGain replay_gain;
            for (auto& field : field_map) {
                if (field.first == kReplaygainAlbumGain) {
                    replay_gain.album_gain = std::stod(field.second[0].to8Bit());
                    found = true;
                }
                else if (field.first == kReplaygainTrackPeak) {
                    replay_gain.track_peak = std::stod(field.second[0].to8Bit());
                    found = true;
                }
                else if (field.first == kReplaygainAlbumPeak) {
                    replay_gain.album_peak = std::stod(field.second[0].to8Bit());
                    found = true;
                }
                else if (field.first == kReplaygainTrackGain) {
                    replay_gain.track_gain = std::stod(field.second[0].to8Bit());
                    found = true;
                }
                else if (field.first == kReplaygainReferenceLoudness) {
                    replay_gain.ref_loudness = std::stod(field.second[0].to8Bit());
                    found = true;
                }
            }
            if (!found) {
                return std::unexpected(ParseMetadataError::PARSE_ERROR_NOT_FOUND);
            }
            return replay_gain;
        }
    };

    const HashMap<std::string_view, std::function<ScopedPtr<IFileTagReader>()>>
        kFileTagReaderLut{
        { ".flac", [] { return MakeAlign<IFileTagReader, FlacTagReader>(); } },
        { ".mp3",  [] { return MakeAlign<IFileTagReader, Mp3TagReader>(); } },
        { ".m4a",  [] { return MakeAlign<IFileTagReader, Mp4TagReader>(); } },
        { ".mp4",  [] { return MakeAlign<IFileTagReader, Mp4TagReader>(); } },
	    { ".opus", [] { return MakeAlign<IFileTagReader, OpusTagReader>(); } },
        { ".dff",  [] { return MakeAlign<IFileTagReader, DiffTagReader>(); } },
		{ ".dsf",  [] { return MakeAlign<IFileTagReader, DsfTagReader>(); } }
    };

    ScopedPtr<IFileTagReader> MakeFileTagReader(const std::string& ext) {
        const auto itr = kFileTagReaderLut.find(ext);
        if (itr != kFileTagReaderLut.end()) {
            return itr->second();
        }
        return nullptr;
    }

    void SetFileInfo(const Path& path, TrackInfo& track_info) {
        track_info.file_path = path;        
        try {
            track_info.file_size = Fs::file_size(path);
        }
        catch (...) {
        }
    }

    void SetAudioProperties(const AudioProperties* audio_properties, TrackInfo& track_info) {
        if (audio_properties != nullptr) {
            track_info.duration = audio_properties->lengthInMilliseconds() / 1000.0;
            track_info.bit_rate = audio_properties->bitrate();
            track_info.sample_rate = audio_properties->sampleRate();
        }
    }

    void ExtractTitleFromFileName(TrackInfo& track_info) {
        std::optional<std::wstring> file_name_no_ext(track_info.file_name_no_ext());

        if (file_name_no_ext) {
            std::wregex pattern(L"Track(\\d{2})\\.(.*)");
            std::wsmatch matches;

            if (std::regex_search(file_name_no_ext.value(), matches, pattern)) {
                track_info.track = std::stoi(matches[1].str());
				if (track_info.track > kMaxTrackNumber) {
					track_info.track = 0;
				}
                track_info.title = matches[2].str();
            }
            else {
                track_info.title = file_name_no_ext.value();
            }
        }
    }

    void ExtractTag(Path const& path, const Tag* tag, const AudioProperties* audio_properties, TrackInfo& track_info) {
        try {
            if (!tag->isEmpty()) {
                if (!tag->artist().toWString().empty()) {
                    track_info.artist = tag->artist().toWString();
                }
                if (!tag->title().toWString().empty()) {
                    track_info.title = tag->title().toWString();
                }
                if (!tag->album().toWString().empty()) {
                    track_info.album = tag->album().toWString();
                }
                track_info.track = tag->track();
                track_info.year = tag->year();
                // 如果讀取到年分是20211126
                if (track_info.year > 0) {
                    if (track_info.year > 10000) {
                        track_info.year = track_info.year / 10000;
                    }
                    else if (track_info.year <= 1900) {
                        track_info.year = 1990;
                    }
                }
                else {
                    track_info.year = 0;
                }
                if (!tag->genre().toWString().empty()) {
                    track_info.genre = tag->genre().toWString();
                }
                if (!tag->comment().toWString().empty()) {
                    track_info.comment = tag->comment().toWString();
                }
            }
        }
        catch (const std::exception& e) {
            XAMP_LOG_DEBUG("ExtractTag path: {}", e.what());
        }        
    }
}

class TaglibHelper {
public:
    friend class SharedSingleton<TaglibHelper>;

	[[nodiscard]] HashSet<std::string> const & GetSupportFileExtensions() const noexcept {
		return support_file_extensions_;
	}

    [[nodiscard]] bool IsSupported(const std::string& file_ext) const noexcept {
        return support_file_extensions_.find(file_ext) != support_file_extensions_.end();
    }

protected:
	TaglibHelper() {
		for (const auto& file_exts : FileRef::defaultFileExtensions()) {
			support_file_extensions_.insert(std::string(".") + file_exts.toCString());
		}
        support_file_extensions_.erase(".mkv");
        support_file_extensions_.erase(".m4v");
	}
private:
    HashSet<std::string> support_file_extensions_;
};

class LibarchiveIOStream : public TagLib::IOStream {
public:
    explicit LibarchiveIOStream(ArchiveEntry entry)
        : entry(std::move(entry)) {
        ensureCached();
    }

    FileName name() const {
        return entry.Name().c_str();
    }

    void ensureCached() {
        if (!buffer_.empty()) return;

        if (entry.Length() <= 0)
            throw std::runtime_error("ArchiveEntry length unknown or zero.");

        buffer_.resize(static_cast<size_t>(entry.Length()));

        long total_read = 0;
        while (total_read < entry.Length()) {
            auto chunk = entry.Read(buffer_.data() + total_read,
                static_cast<long>(entry.Length() - total_read));
            if (!chunk) {
                throw std::runtime_error(chunk.error().c_str());
            }
            if (chunk.value() <= 0)
                throw std::runtime_error("ArchiveEntry Read failed or truncated.");
            total_read += chunk.value();
        }
        pos_ = 0;
    }

    void writeBlock(const TagLib::ByteVector&) override {
    }

    void insert(const ByteVector& data, offset_t start = 0, size_t replace = 0) override {
    }

    void removeBlock(offset_t start = 0, size_t length = 0) override {
    }

    bool readOnly() const override {
        return true;
    }

    bool isOpen() const override { 
        return !buffer_.empty(); 
    }

    long long length() override {
        return static_cast<long long>(buffer_.size());
    }

    ByteVector readBlock(size_t length) {
        ensureCached();

        const size_t avail = buffer_.size() - static_cast<size_t>(pos_);
        const size_t n = std::min<size_t>(length, avail);

        ByteVector out(buffer_.data() + pos_, n);
        pos_ += static_cast<long long>(n);
        return out;
    }

    void seek(long long offset, Position p) override {
        ensureCached();

        long long base = 0;
        switch (p) {
        case Beginning: base = 0;              break;
        case Current:   base = pos_;           break;
        case End:       base = buffer_.size(); break;
        default:        return;
        }
        long long newPos = base + offset;
        if (newPos < 0 || newPos > static_cast<long long>(buffer_.size()))
            return;
        pos_ = newPos;
    }

    long long tell() const override {
        return pos_; 
    }

    void truncate(long long) override {
    }
private:
    long pos_;
    ArchiveEntry entry;
    std::vector<char> buffer_;
};

class TaglibMetadataReader::TaglibMetadataReaderImpl {
public:
    void Open(ArchiveEntry entry) {
        auto archive_path = entry.ArchivePath();
        archive_stream_ = MakeAlign<LibarchiveIOStream>(std::move(entry));
        FileRef fileref(archive_stream_.get(), true, TagLib::AudioProperties::Fast);
        if (!fileref.isNull()) {
            fileref_opt_ = fileref;
            path_ = archive_path;
            is_archive_file_ = true;
        }
    }

	void Open(const Path& path) {
        PrefetchFile(path);
        stream_ = MakeAlign<TaglibIOStream>(path, true);
        FileRef fileref(stream_.get(), true, TagLib::AudioProperties::Fast);
//#ifdef XAMP_OS_WIN
//        FileRef fileref(path.wstring().c_str(), true, TagLib::AudioProperties::Fast);
//#else
//        FileRef fileref(path.string().c_str(), true, TagLib::AudioProperties::Fast);
//#endif
        if (!fileref.isNull()) {
            fileref_opt_ = fileref;
            path_ = path;            
            file_ext_ = String::ToLower(path_.extension().string());
            tag_reader_ = MakeFileTagReader(file_ext_);
        }
	}

    std::expected<TrackInfo, ParseMetadataError> Extract() const {
        TrackInfo track_info;

		if (!fileref_opt_) {            
            return std::unexpected(ParseMetadataError::PARSE_ERROR_OPEN_FILE);
		}

        const auto& file_ref = *fileref_opt_;
        const auto* tag = file_ref.tag();

        if (is_archive_file_) {
            track_info.file_size = archive_stream_->length();
            track_info.archive_entry_name = archive_stream_->name().toString().toCWString();
            track_info.file_path = path_;
            track_info.is_zip_file = true;
            if (tag != nullptr) {
                ExtractTag(path_, tag, file_ref.audioProperties(), track_info);
            }
            SetAudioProperties(file_ref.audioProperties(), track_info);
        }
        else {
            if (tag != nullptr) {
                ExtractTag(path_, tag, file_ref.audioProperties(), track_info);
            }
            SetFileInfo(path_, track_info);
            SetAudioProperties(file_ref.audioProperties(), track_info);
        }

        // Tag not empty but title maybe empty!
        if (tag) {
            if (track_info.title.empty()) {
                ExtractTitleFromFileName(track_info);
            }
        }
        auto replay_gain = ReadReplayGain(file_ref.file());
        if (replay_gain) {
            track_info.replay_gain = replay_gain.value();
        }
        return track_info;
    }

    std::expected<std::vector<std::byte>, ParseMetadataError> ReadEmbeddedCover() const {
		if (!IsSupported()) {
            return std::unexpected(ParseMetadataError::PARSE_ERROR_NOT_SUPPORT);
		}

        if (!fileref_opt_ || !fileref_opt_->tag()) {
            return std::unexpected(ParseMetadataError::PARSE_ERROR_OPEN_FILE);
        }
        return ReadEmbeddedCover(fileref_opt_->file());
    }

    XAMP_ALWAYS_INLINE bool IsSupported() const noexcept {
		return SharedSingleton<TaglibHelper>::GetInstance().IsSupported(file_ext_);
    }

    std::expected<ReplayGain, ParseMetadataError> ReadReplayGain() const {
        if (!fileref_opt_) {
            return std::unexpected(ParseMetadataError::PARSE_ERROR_OPEN_FILE);
        }
        return ReadReplayGain(fileref_opt_->file());
    }

private:
    std::expected<ReplayGain, ParseMetadataError> ReadReplayGain(File* file) const {
        if (tag_reader_ != nullptr) {
            return tag_reader_->ReadReplayGain(file);
        }
        return std::unexpected(ParseMetadataError::PARSE_ERROR_NOT_FOUND);
    }

    std::expected<std::vector<std::byte>, ParseMetadataError> ReadEmbeddedCover(File* file) const {
        if (tag_reader_ != nullptr) {
            return tag_reader_->ReadEmbeddedCover(file);
        }
        return std::unexpected(ParseMetadataError::PARSE_ERROR_NOT_FOUND);
    }

    bool is_archive_file_{ false };
    std::string file_ext_;
    Path path_;
    std::optional<FileRef> fileref_opt_;
    ScopedPtr<IFileTagReader> tag_reader_;
    ScopedPtr<LibarchiveIOStream> archive_stream_;
    ScopedPtr<TaglibIOStream> stream_;
};

XAMP_PIMPL_IMPL(TaglibMetadataReader)

TaglibMetadataReader::TaglibMetadataReader()
    : reader_(MakeAlign<TaglibMetadataReaderImpl>()) {
}

void TaglibMetadataReader::Open(ArchiveEntry archive_entry) {
    return reader_->Open(std::move(archive_entry));
}

void TaglibMetadataReader::Open(const Path& path) {
    return reader_->Open(path);
}

std::expected<TrackInfo, ParseMetadataError> TaglibMetadataReader::Extract() {
    return reader_->Extract();
}

std::expected<ReplayGain, ParseMetadataError> TaglibMetadataReader::ReadReplayGain() {
    return reader_->ReadReplayGain();
}

std::expected<std::vector<std::byte>, ParseMetadataError> TaglibMetadataReader::ReadEmbeddedCover() {
    return reader_->ReadEmbeddedCover();
}

const HashSet<std::string>& TaglibMetadataReader::GetSupportFileExtensions() {
    return SharedSingleton<TaglibHelper>::GetInstance().GetSupportFileExtensions();
}

bool TaglibMetadataReader::IsSupported() const {
    return reader_->IsSupported();
}

XAMP_METADATA_NAMESPACE_END
