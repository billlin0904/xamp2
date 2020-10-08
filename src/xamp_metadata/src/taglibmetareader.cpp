#include <vector>
#include <sstream>
#include <functional>

#include <base/stl.h>
#include <base/str_utilts.h>
#include <base/memory.h>
#include <base/logger.h>

#include <metadata/taglib.h>
#include <metadata/taglibmetareader.h>

namespace xamp::metadata {

static bool GetID3V2TagCover(TagLib::ID3v2::Tag* tag, std::vector<uint8_t>& buffer) {
    if (!tag) {
        return false;
    }
    auto const & frame_list = tag->frameList("APIC");
    if (frame_list.isEmpty()) {
        return false;
    }

    const auto frame = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame*>(frame_list.front());
    if (!frame) {
        return false;
    }
    buffer.resize(frame->picture().size());
    (void) FastMemcpy(buffer.data(), frame->picture().data(), static_cast<int32_t>(frame->picture().size()));
    return true;
}

static bool GetApeTagCover(TagLib::APE::Tag* tag, std::vector<uint8_t>& buffer) {
    auto const & listMap = tag->itemListMap();

    if (!listMap.contains("COVER ART (FRONT)")) {
        return false;
    }

    const TagLib::ByteVector nullStringTerminator(1, 0);
    auto item = listMap["COVER ART (FRONT)"].binaryData();
    auto pos = item.find(nullStringTerminator);	// Skip the filename
    if (++pos > 0) {
        auto pic = item.mid(pos);
        buffer.resize(pic.size());
        (void) FastMemcpy(buffer.data(), pic.data(), static_cast<int32_t>(pic.size()));
        return true;
    }
    return false;
}

static bool GetMp3Cover(File* file, std::vector<uint8_t>& buffer) {
    bool found = false;
    if (auto mpeg_file = dynamic_cast<TagLib::MPEG::File*>(file)) {
        if (mpeg_file->ID3v2Tag()) {
            found = GetID3V2TagCover(mpeg_file->ID3v2Tag(), buffer);
        }
        if (!found && mpeg_file->APETag()) {
            GetApeTagCover(mpeg_file->APETag(), buffer);
        }
    }
    return found;
}

static bool GetApeCover(File* file, std::vector<uint8_t>& buffer) {
    if (const auto ape_file = dynamic_cast<TagLib::APE::File*>(file)) {
        return GetApeTagCover(ape_file->APETag(), buffer);
    }
    return false;
}

static bool GetMp4Cover(File* file, std::vector<uint8_t>& buffer) {
    if (auto mp4_file = dynamic_cast<TagLib::MP4::File*>(file)) {
        auto tag = mp4_file->tag();
        if (!tag) {
            return false;
        }

        if (!tag->itemListMap().contains("covr")) {
            return false;
        }

        auto cover_list = tag->itemListMap()["covr"].toCoverArtList();
        if (cover_list.isEmpty()) {
            return false;
        }
        if (cover_list[0].data().size() > 0) {
            buffer.resize(cover_list[0].data().size());
            (void) FastMemcpy(buffer.data(), cover_list[0].data().data(),
                static_cast<int32_t>(cover_list[0].data().size()));
            return true;
        }
    }
    return false;
}

static bool GetFlacCover(File* file, std::vector<uint8_t>& buffer) {
    if (auto flac_file = dynamic_cast<TagLib::FLAC::File*>(file)) {
        const auto picture_list = flac_file->pictureList();
        if (picture_list.isEmpty()) {
            return false;
        }

        for (const auto &picture : picture_list) {
            if (picture->type() == TagLib::FLAC::Picture::FrontCover) {                
                buffer.resize(picture->data().size());
                (void)FastMemcpy(buffer.data(), picture->data().data(), picture->data().size());
                return true;
            }
        }
    }
    return false;
}

static void SetFileInfo(Path const & path, Metadata& metadata) {
    metadata.file_path = path.wstring();
    metadata.file_name = path.filename().wstring();
    metadata.parent_path = path.parent_path().wstring();
    metadata.file_ext = path.extension().wstring();
    metadata.file_name_no_ext = path.stem().wstring();
}

static void SetAudioProperties(AudioProperties* audio_properties, Metadata& metadata) {
    if (audio_properties != nullptr) {
        metadata.duration = audio_properties->lengthInMilliseconds() / 1000.0;
        metadata.bitrate = audio_properties->bitrate();
		metadata.samplerate = audio_properties->sampleRate();
    }
}

static void ExtractTitleFromFileName(Metadata &metadata) {
    const auto start_pos = metadata.file_name_no_ext.find(L'.');
    if (start_pos != std::wstring::npos) {
        metadata.title = metadata.file_name_no_ext.substr(start_pos + 1);
        std::wistringstream istr(metadata.file_name_no_ext.substr(0, start_pos));
        istr >> metadata.title >> metadata.track;
    }
    else {
        metadata.title = metadata.file_name_no_ext;
    }
}

static void ExtractTag(Path const & path, Tag* tag, AudioProperties*audio_properties, Metadata& metadata) {
    try {
        if (!tag->isEmpty()) {
            metadata.artist = tag->artist().toWString();
            metadata.title = tag->title().toWString();
            metadata.album = tag->album().toWString();
            metadata.track = tag->track();
        }
    }
    catch (const std::exception& e) {
        XAMP_LOG_DEBUG("ExtractTag path: {}", e.what());
    }    

    SetFileInfo(path, metadata);
    SetAudioProperties(audio_properties, metadata);
}

class TaglibHelper {
public:
	static TaglibHelper& Instance() {
		static TaglibHelper instance;
		return instance;
	}

    HashSet<std::string> const & GetSupportFileExtensions() const noexcept {
		return support_file_extensions_;
	}

	[[nodiscard]] bool IsSupported(Path const & path) const noexcept {
		const auto file_ext = ToLower(path.extension().string());
		return support_file_extensions_.find(file_ext) != support_file_extensions_.end();
	}

protected:
	TaglibHelper() {
		for (const auto& file_exts : TagLib::FileRef::defaultFileExtensions()) {
			support_file_extensions_.insert(std::string(".") + file_exts.toCString());
		}
	}
private:
    HashSet<std::string> support_file_extensions_;
};

class TaglibMetadataReader::TaglibMetadataReaderImpl {
public:
    TaglibMetadataReaderImpl() = default;

    Metadata Extract(const Path& path) const {
#ifdef XAMP_OS_WIN
        FileRef fileref(path.wstring().c_str(), true, TagLib::AudioProperties::Fast);
#else
        FileRef fileref(path.string().c_str(), true, TagLib::AudioProperties::Fast);
#endif
        const auto tag = fileref.tag();

        Metadata metadata;

        if (tag != nullptr) {
            ExtractTag(path, tag, fileref.audioProperties(), metadata);
        } else {            
            SetFileInfo(path, metadata);
            SetAudioProperties(fileref.audioProperties(), metadata);
        }

        // Tag not empty but title maybe empty!
        if (metadata.title.empty()) {
            ExtractTitleFromFileName(metadata);
        }

        return metadata;
    }

    std::vector<uint8_t> const & ExtractCover(Path const & path) {
        cover_.clear();

		if (!IsSupported(path)) {
			cover_.clear();
			return cover_;
		}

#ifdef XAMP_OS_WIN
        FileRef fileref(path.wstring().c_str());
#else
        FileRef fileref(path.string().c_str());
#endif
        const auto tag = fileref.tag();
        if (!tag) {
            cover_.clear();
            return cover_;
        }
        const auto ext = ToLower(path.extension().string());
        GetCover(ext, fileref.file(), cover_);
        return cover_;
    }

    HashSet<std::string> const & GetSupportFileExtensions() const {
        return TaglibHelper::Instance().GetSupportFileExtensions();
    }

    bool IsSupported(Path const & path) const noexcept {
		return TaglibHelper::Instance().IsSupported(path);
    }

private:
    static void GetCover(std::string const & ext, File*file, std::vector<uint8_t>& cover) {
        static const HashMap<std::string, std::function<bool(File *, std::vector<uint8_t> &)>>
            parse_cover_table{
            { ".flac", GetFlacCover },
            { ".mp3",  GetMp3Cover },
            { ".m4a",  GetMp4Cover },
            { ".ape",  GetApeCover },
        };
        auto itr = parse_cover_table.find(ext);
        if (itr != parse_cover_table.end()) {
            (*itr).second(file, cover);
        }
    }

    std::vector<uint8_t> cover_;
};

XAMP_PIMPL_IMPL(TaglibMetadataReader)

TaglibMetadataReader::TaglibMetadataReader()
    : reader_(MakeAlign<TaglibMetadataReaderImpl>()) {
}

Metadata TaglibMetadataReader::Extract(const Path& path) {
    return reader_->Extract(path);
}

const std::vector<uint8_t>& TaglibMetadataReader::ExtractEmbeddedCover(Path const & path) {
    return reader_->ExtractCover(path);
}

const HashSet<std::string>& TaglibMetadataReader::GetSupportFileExtensions() const {
    return reader_->GetSupportFileExtensions();
}

bool TaglibMetadataReader::IsSupported(Path const & path) const {
    return reader_->IsSupported(path);
}

}
