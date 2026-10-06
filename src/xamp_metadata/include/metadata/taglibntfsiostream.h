#pragma once

#include <cstdio>
#include <limits>
#include <stdexcept>

#include <base/ntfsfilestream.h>
#include <base/fs.h>
#include <base/str_utilts.h>
#include <metadata/metadata.h>
#include <metadata/taglib.h>

XAMP_METADATA_NAMESPACE_BEGIN

// TagLib borrows this stream; its owner must outlive the FileRef.
class TaglibNtfsIOStream final : public TagLib::IOStream {
public:
	explicit TaglibNtfsIOStream(const Path& path)
		: stream_(path) {
#ifdef XAMP_OS_WIN
		name_ = path.wstring();
#else
		name_ = String::toUtf8String(path.wstring());
#endif
	}

	TagLib::FileName name() const override {
		return name_.c_str();
	}
	bool readOnly() const override {
		return true;
	}
	bool isOpen() const override {
		return true;
	}

	TagLib::ByteVector readBlock(size_t length) override {
		if (length == 0) {
			return {};
		}
		if (length > (std::numeric_limits<uint32_t>::max)()) {
			throw std::length_error("TagLib NTFS read exceeds ByteVector capacity");
		}
		TagLib::ByteVector data;
		data.resize(static_cast<uint32_t>(length));
		const auto received = stream_.read(data.data(), length);
		data.resize(static_cast<uint32_t>(received));
		return data;
	}

	void seek(TagLib::offset_t offset, Position position = Beginning) override {
		int origin;
		switch (position) {
		case Beginning:
			origin = SEEK_SET;
			break;
		case Current:
			origin = SEEK_CUR;
			break;
		case End:
			origin = SEEK_END;
			break;
		default:
			throw std::invalid_argument("Invalid TagLib NTFS seek origin");
		}
		stream_.seek(offset, origin);
	}

	TagLib::offset_t tell() const override {
		return static_cast<TagLib::offset_t>(stream_.tell());
	}
	TagLib::offset_t length() override {
		return static_cast<TagLib::offset_t>(stream_.size());
	}
	void writeBlock(const TagLib::ByteVector&) override {
		denyWrite();
	}
	void insert(const TagLib::ByteVector&, TagLib::offset_t = 0, size_t = 0) override {
		denyWrite();
	}
	void removeBlock(TagLib::offset_t = 0, size_t = 0) override {
		denyWrite();
	}
	void truncate(TagLib::offset_t) override {
		denyWrite();
	}

private:
	[[noreturn]] static void denyWrite() {
		throw std::logic_error("NTFS metadata stream is read-only");
	}

	NtfsFileStream stream_;
#ifdef XAMP_OS_WIN
	std::wstring name_;
#else
	std::string name_;
#endif
};

XAMP_METADATA_NAMESPACE_END
