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

	bool IsFileReadOnly(Path const & path) const {
		namespace Fs = std::filesystem;
		return (Fs::status(path).permissions() & Fs::perms::owner_read) != Fs::perms::none;		
	}

    void Write(Path const &path, Metadata &metadata) const {
        Write(path, [metadata](auto, auto tag) {
			tag->setAlbum(metadata.album);
			tag->setArtist(metadata.artist);
			tag->setTrack(metadata.track);
			tag->setTitle(metadata.title);
		});
    }

    void WriteTitle(Path const& path, std::wstring const &title) const {
        Write(path, [title](auto, auto tag) {
	        tag->setTitle(title);
	    });
    }

    void WriteArtist(Path const& path, std::wstring const &artist) const {
        Write(path, [artist](auto, auto tag) {
	        tag->setArtist(artist);
	    });
    }

    void WriteAlbum(Path const& path, std::wstring const &album) const {
        Write(path, [album](auto, auto tag) {
	        tag->setAlbum(album);
	    });
    }

    void WriteTrack(Path const& path, int32_t track) const {
        Write(path, [track](auto, auto tag) {
	        tag->setTrack(track);
	    });
    }

	void WriteEmbeddedCover(Path const& path, std::vector<uint8_t> const & image) const {
		const auto ext = ToLower(path.extension().string());

		const TagLib::ByteVector imagedata(reinterpret_cast<const char *>(image.data()), image.size());

		if (ext == ".m4a") {			
			TagLib::MP4::CoverArt cover_art(static_cast<TagLib::MP4::CoverArt::Format>(0x0D), imagedata);

            Write(path, [&cover_art](auto, auto tag) {
				if (const auto mp4_tag = dynamic_cast<TagLib::MP4::Tag*>(tag)) {
					auto items_list_map = mp4_tag->itemListMap();
					TagLib::MP4::CoverArtList cover_art_list;
					cover_art_list.append(cover_art);
					const TagLib::MP4::Item cover_item(cover_art_list);
					items_list_map.insert("covr", cover_item);
				}
			});
		}
		else if (ext == ".mp3") {
            Write(path, [&imagedata](auto file, auto) {
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
		} else if (ext == ".flac") {
            Write(path, [&imagedata](auto file, auto) {
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
    static void Write(Path const& path, Function &&fun) {
#ifdef XAMP_OS_WIN
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

bool TaglibMetadataWriter::IsFileReadOnly(Path const & path) const {
	return writer_->IsFileReadOnly(path);
}

void TaglibMetadataWriter::WriteTitle(Path const & path, std::wstring const & title) const {
    writer_->WriteTitle(path, title);
}

void TaglibMetadataWriter::Write(Path const & path, Metadata& metadata) {
    writer_->Write(path, metadata);
}

void TaglibMetadataWriter::WriteArtist(Path const & path, std::wstring const & artist) const {
    writer_->WriteArtist(path, artist);
}

void TaglibMetadataWriter::WriteAlbum(Path const & path, std::wstring const & album) const {
    writer_->WriteAlbum(path, album);
}

void TaglibMetadataWriter::WriteTrack(Path const & path, int32_t track) const {
    writer_->WriteTrack(path, track);
}

void TaglibMetadataWriter::WriteEmbeddedCover(Path const & path, std::vector<uint8_t> const & image) const {
	writer_->WriteEmbeddedCover(path, image);
}

}
