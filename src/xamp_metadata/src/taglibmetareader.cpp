#include <metadata/taglibmetareader.h>

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
    double ParseStringList(const std::string & s, bool string_dummy = true) {
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

    std::optional<ReplayGain> GetMp3ReplayGain(File* file) {
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
            return std::nullopt;
        }
        return CreateOptional<ReplayGain>(std::move(replay_gain));
    }

    std::optional<Vector<std::byte>> GetID3V2TagCover(const ID3v2::Tag* tag) {
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
        Vector<std::byte> buffer;
        buffer.resize(frame->picture().size());
        MemoryCopy(buffer.data(), frame->picture().data(), static_cast<int32_t>(frame->picture().size()));
        return CreateOptional<Vector<std::byte>>(std::move(buffer));
    }

    std::optional<Vector<std::byte>> GetApeTagCover(const APE::Tag* tag) {
        auto const& list_map = tag->itemListMap();

        if (!list_map.contains("COVER ART (FRONT)")) {
            return std::nullopt;
        }
        Vector<std::byte> buffer;
        const ByteVector null_string_terminator(1, 0);
        auto item = list_map["COVER ART (FRONT)"].binaryData();
        auto pos = item.find(null_string_terminator);	// Skip the filename
        if (++pos > 0) {
            auto pic = item.mid(pos);
            buffer.resize(pic.size());
            MemoryCopy(buffer.data(), pic.data(), static_cast<int32_t>(pic.size()));
            return CreateOptional<Vector<std::byte>>(std::move(buffer));
        }
        return std::nullopt;
    }

    std::optional<Vector<std::byte>> GetMp3Cover(File* file) {
        std::optional<Vector<std::byte>> buffer;
        if (auto* mpeg_file = dynamic_cast<TagLib::MPEG::File*>(file)) {
            if (mpeg_file->ID3v2Tag()) {
                buffer = GetID3V2TagCover(mpeg_file->ID3v2Tag());
            }
            if (!buffer && mpeg_file->APETag()) {
                buffer = GetApeTagCover(mpeg_file->APETag());
            }
        }
        return buffer;
    }

    std::optional<Vector<std::byte>> GetDsfCover(File* file) {
        if (const auto* dsd_file = dynamic_cast<TagLib::DSF::File*>(file)) {
            if (dsd_file->tag()) {
                return GetID3V2TagCover(dsd_file->tag());
            }
        }
        return std::nullopt;
    }

    std::optional<Vector<std::byte>> GetDsdiffCover(File* file) {
        if (const auto* dsd_file = dynamic_cast<TagLib::DSDIFF::File*>(file)) {
            if (dsd_file->ID3v2Tag()) {
                return GetID3V2TagCover(dsd_file->ID3v2Tag());
            }
        }
        return std::nullopt;
    }

    std::optional<Vector<std::byte>> GetMp4Cover(File* file) {
        Vector<std::byte> buffer;

        if (const auto* mp4_file = dynamic_cast<TagLib::MP4::File*>(file)) {
            auto* tag = mp4_file->tag();
            if (!tag) {
                return std::nullopt;
            }

            if (!tag->itemMap().contains("covr")) {
                return std::nullopt;
            }

            auto cover_list = tag->itemMap()["covr"].toCoverArtList();
            if (cover_list.isEmpty()) {
                return std::nullopt;
            }
            if (cover_list[0].data().size() > 0) {
                buffer.resize(cover_list[0].data().size());
                MemoryCopy(buffer.data(), cover_list[0].data().data(),
                    static_cast<int32_t>(cover_list[0].data().size()));
                return CreateOptional<Vector<std::byte>>(std::move(buffer));
            }
        }
        return std::nullopt;
    }

    std::optional<ReplayGain> GetMp4ReplayGain(File* file) {
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
            return std::nullopt;
        }
        return CreateOptional<ReplayGain>(std::move(replay_gain));
    }

    std::optional<ReplayGain> GetFlacReplayGain(File* file) {
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
            return std::nullopt;
        }
        return CreateOptional<ReplayGain>(std::move(replay_gain));
    }

    std::optional<Vector<std::byte>> GetFlacCover(File* file) {
        if (auto* flac_file = dynamic_cast<TagLib::FLAC::File*>(file)) {
            const auto picture_list = flac_file->pictureList();
            if (picture_list.isEmpty()) {
                return std::nullopt;
            }
            Vector<std::byte> buffer;
            for (const auto& picture : picture_list) {
                if (picture->type() == TagLib::FLAC::Picture::FrontCover) {
                    buffer.resize(picture->data().size());
                    MemoryCopy(buffer.data(), picture->data().data(), picture->data().size());
                    return CreateOptional<Vector<std::byte>>(std::move(buffer));
                }
            }
        }
        return std::nullopt;
    }

    void SetFileInfo(const Path& path, TrackInfo& track_info) {
        track_info.file_path = path;
        track_info.file_size = Fs::file_size(path);
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

        SetFileInfo(path, track_info);
        SetAudioProperties(audio_properties, track_info);
    }

    std::optional<ReplayGain> GetReplayGainFromFile(const std::string & ext, File* file) {
        static const HashMap<std::string_view, std::function<std::optional<ReplayGain>(File*)>>
            parse_replay_gain_table{
            { ".flac", GetFlacReplayGain },
            { ".mp3",  GetMp3ReplayGain },
            { ".m4a",  GetMp4ReplayGain },
            { ".mp4",  GetMp4ReplayGain },
        };
        const auto itr = parse_replay_gain_table.find(ext);
        if (itr != parse_replay_gain_table.end()) {
            return std::invoke(itr->second, file);
        }
        return std::nullopt;
    }

    std::optional<Vector<std::byte>> GetCover(const std::string & ext, File* file) {
        static const HashMap<std::string_view, std::function<std::optional<Vector<std::byte>> (File*)>>
            parse_cover_table{
            { ".flac", GetFlacCover },
            { ".mp3",  GetMp3Cover },
            { ".m4a",  GetMp4Cover },
            { ".mp4",  GetMp4Cover },
            { ".dff",  GetDsdiffCover },
            { ".dsf",  GetDsfCover }
        };
        auto itr = parse_cover_table.find(ext);
        if (itr != parse_cover_table.end()) {
            return std::invoke(itr->second, file);
        }
		return std::nullopt;
    }
}

class TaglibHelper {
public:
    friend class Singleton<TaglibHelper>;

	XAMP_NO_DISCARD HashSet<std::string> const & GetSupportFileExtensions() const noexcept {
		return support_file_extensions_;
	}

	XAMP_NO_DISCARD bool IsSupported(const Path & path) const noexcept {
		const auto file_ext = String::ToLower(path.extension().string());
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

class TaglibMetadataReader::TaglibMetadataReaderImpl {
public:
	void Open(const Path& path) {
		file_ref_opt_ = TryGetFileRef(path);
        path_ = path;
	}

    XAMP_NO_DISCARD TrackInfo Extract() const {
		if (!file_ref_opt_) {
			throw Exception("file was NULL!");
		}

        const auto& file_ref = *file_ref_opt_;
        const auto* tag = file_ref.tag();

        TrackInfo track_info;

        if (tag != nullptr) {
            ExtractTag(path_, tag, file_ref.audioProperties(), track_info);
        } else {            
            SetFileInfo(path_, track_info);
            SetAudioProperties(file_ref.audioProperties(), track_info);
        }

        // Tag not empty but title maybe empty!
        if (tag) {
            if (track_info.title.empty()) {
                ExtractTitleFromFileName(track_info);
            }
        }

        const auto ext = String::ToLower(path_.extension().string());
        track_info.replay_gain = GetReplayGainFromFile(ext, file_ref.file());
        return track_info;
    }

    std::optional<Vector<std::byte>> ReadEmbeddedCover() const {
		if (!IsSupported()) {
            return std::nullopt;
		}

        if (!file_ref_opt_ || !file_ref_opt_->tag()) {
            return std::nullopt;
        }
        const auto ext = String::ToLower(path_.extension().string());
        return GetCover(ext, file_ref_opt_->file());
    }

    XAMP_ALWAYS_INLINE bool IsSupported() const noexcept {
		return Singleton<TaglibHelper>::GetInstance().IsSupported(path_);
    }

    std::optional<ReplayGain> GetReplayGain() const {
        if (!file_ref_opt_) {
            return std::nullopt;
        }
        const auto ext = String::ToLower(path_.extension().string());
        return GetReplayGainFromFile(ext, file_ref_opt_->file());
    }

private:
    static std::optional<FileRef> TryGetFileRef(const Path& path) {
#ifdef XAMP_OS_WIN
        FileRef ref(path.wstring().c_str(), true, TagLib::AudioProperties::Fast);
#else
        FileRef ref(path.string().c_str(), true, TagLib::AudioProperties::Fast);
#endif
        return ref.isNull() ? std::nullopt : CreateOptional<FileRef>(std::move(ref));
    }

    Path path_;
    std::optional<FileRef> file_ref_opt_;
};

XAMP_PIMPL_IMPL(TaglibMetadataReader)

TaglibMetadataReader::TaglibMetadataReader()
    : reader_(MakeAlign<TaglibMetadataReaderImpl>()) {
}

void TaglibMetadataReader::Open(const Path& path) {
    return reader_->Open(path);
}

TrackInfo TaglibMetadataReader::Extract() {
    return reader_->Extract();
}

std::optional<ReplayGain> TaglibMetadataReader::GetReplayGain() {
    return reader_->GetReplayGain();
}

std::optional<Vector<std::byte>> TaglibMetadataReader::ReadEmbeddedCover() {
    return reader_->ReadEmbeddedCover();
}

const HashSet<std::string>& TaglibMetadataReader::GetSupportFileExtensions() {
    return Singleton<TaglibHelper>::GetInstance().GetSupportFileExtensions();
}

bool TaglibMetadataReader::IsSupported() const {
    return reader_->IsSupported();
}

XAMP_METADATA_NAMESPACE_END
