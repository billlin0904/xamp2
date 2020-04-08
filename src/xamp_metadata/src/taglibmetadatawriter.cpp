#include <functional>

#include <base/windows_handle.h>
#include <base/str_utilts.h>
#include <metadata/taglib.h>
#include <metadata/taglibmetawriter.h>

namespace xamp::metadata {

class TaglibMetadataWriter::TaglibMetadataWriterImpl {
public:
	TaglibMetadataWriterImpl() {
	}

	bool IsFileReadOnly(const Path& path) const {
#ifdef XAMP_OS_WIN
		auto attr = ::GetFileAttributesW(path.wstring().c_str());
		if (attr != INVALID_FILE_ATTRIBUTES) {
			return attr & FILE_ATTRIBUTE_READONLY;
		}
#endif
		return false;
	}

    void Write(const Path &path, Metadata &metadata) const {
        Write(path, [metadata](auto file, auto tag) {
			tag->setAlbum(metadata.album);
			tag->setArtist(metadata.artist);
			tag->setTrack(metadata.track);
			tag->setTitle(metadata.title);
		});
    }

    void WriteTitle(const Path &path, const std::wstring &title) const {
        Write(path, [title](auto file, auto tag) {
	        tag->setTitle(title);
	    });
    }

    void WriteArtist(const Path &path, const std::wstring &artist) const {
        Write(path, [artist](auto file, auto tag) {
	        tag->setArtist(artist);
	    });
    }

    void WriteAlbum(const Path &path, const std::wstring &album) const {
        Write(path, [album](auto file, auto tag) {
	        tag->setAlbum(album);
	    });
    }

    void WriteTrack(const Path &path, int32_t track) const {
        Write(path, [track](auto file, auto tag) {
	        tag->setTrack(track);
	    });
    }

	void WriteEmbeddedCover(const Path& path, const std::vector<uint8_t>& image) const {
		const auto ext = ToUpper(path.extension().string());

		const TagLib::ByteVector imagedata(reinterpret_cast<const char *>(image.data()), image.size());

		if (ext == ".M4A") {			
			TagLib::MP4::CoverArt cover_art(static_cast<TagLib::MP4::CoverArt::Format>(0x0D), imagedata);

            Write(path, [&cover_art](auto file, auto tag) {
				if (const auto mp4_tag = dynamic_cast<TagLib::MP4::Tag*>(tag)) {
					auto items_list_map = mp4_tag->itemListMap();
					TagLib::MP4::CoverArtList cover_art_list;
					cover_art_list.append(cover_art);
					const TagLib::MP4::Item cover_item(cover_art_list);
					items_list_map.insert("covr", cover_item);
				}
			});
		}
		else if (ext == ".MP3") {
            Write(path, [&imagedata](auto file, auto tag) {
				if (const auto mp3_file = dynamic_cast<TagLib::MPEG::File*>(file)) {
					if (const auto mp3_tag = mp3_file->ID3v2Tag(true)) {
						const auto frame_list = mp3_tag->frameList("APIC");
						for (auto it : frame_list) {
							const auto frame = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame *>(it);
							if (frame != nullptr) {
								mp3_tag->removeFrame(frame, true);
							}
						}
						auto frame = new TagLib::ID3v2::AttachedPictureFrame("APIC");
						frame->setType(TagLib::ID3v2::AttachedPictureFrame::FrontCover);
						frame->setMimeType("image/jpeg");
						frame->setPicture(imagedata);
						mp3_tag->addFrame(frame);
					}					
				}
			});
		} else if (ext == ".FLAC") {
			Write(path, [&imagedata](auto file, auto tag) {
				if (const auto flac_file = dynamic_cast<TagLib::FLAC::File*>(file)) {
					flac_file->removePictures();
					auto picture = new TagLib::FLAC::Picture();
					picture->setMimeType("image/jpeg");
					picture->setData(imagedata);
					picture->setType(TagLib::FLAC::Picture::FrontCover);
					flac_file->addPicture(picture);
				}
			});
		}
    }

private:
    template <typename Function>
    static void Write(const Path &path, Function &&fun) {
#ifdef _WIN32
        FileRef fileref(path.wstring().c_str());
#else
        FileRef fileref(path.string().c_str());
#endif
        fun(fileref.file(), fileref.tag());
        fileref.save();
    }
};

XAMP_PIMPL_IMPL(TaglibMetadataWriter)

TaglibMetadataWriter::TaglibMetadataWriter()
    : writer_(MakeAlign<TaglibMetadataWriterImpl>()) {
}

bool TaglibMetadataWriter::IsFileReadOnly(const Path& path) const {
	return writer_->IsFileReadOnly(path);
}

void TaglibMetadataWriter::WriteTitle(const Path& path, const std::wstring& title) const {
    writer_->WriteTitle(path, title);
}

void TaglibMetadataWriter::Write(const Path& path, Metadata& metadata) {
    writer_->Write(path, metadata);
}

void TaglibMetadataWriter::WriteArtist(const Path& path, const std::wstring& artist) const {
    writer_->WriteArtist(path, artist);
}

void TaglibMetadataWriter::WriteAlbum(const Path& path, const std::wstring& album) const {
    writer_->WriteAlbum(path, album);
}

void TaglibMetadataWriter::WriteTrack(const Path& path, int32_t track) const {
    writer_->WriteTrack(path, track);
}

void TaglibMetadataWriter::WriteEmbeddedCover(const Path& path, const std::vector<uint8_t>& image) const {
	writer_->WriteEmbeddedCover(path, image);
}

}
