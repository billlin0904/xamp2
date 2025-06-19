//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <metadata/taglib.h>
#include <base/fs.h>
#include <base/stl.h>
#include <base/fastiostream.h>
#include <base/memory.h>

XAMP_METADATA_NAMESPACE_BEGIN

class TaglibIOStream final : public TagLib::IOStream {
public:
	TaglibIOStream() = default;

	virtual ~TaglibIOStream() override = default;

	explicit TaglibIOStream(const Path& path)
		: io_stream_(path, FastIOStream::Mode::ReadWriteOnlyExisting) {
	}

	void open(const Path& path) {
		io_stream_.open(path, FastIOStream::Mode::ReadWriteOnlyExisting);
	}

	TagLib::FileName name() const override {
		return io_stream_.path().wstring().c_str();
	}

	TagLib::ByteVector readBlock(size_t len) override {
		if (!isOpen() || len == 0)
			return {};

		TagLib::ByteVector v;
		v.resize(static_cast<uint32_t>(len));

		size_t n = io_stream_.read(v.data(), len);
		v.resize(static_cast<uint32_t>(n));
		return v;
	}

	void writeBlock(const TagLib::ByteVector& v) override {
		if (v.isEmpty() || io_stream_.read_only())
			return;

		size_t total = 0;
		const char* data = v.data();
		const size_t len = v.size();

		while (total < len) {
			size_t w = io_stream_.write(data + total, len - total);
			if (w == 0) {
				throw PlatformException();
			}
			total += w;
		}
	}

	void insert(const TagLib::ByteVector& data, offset_t start = 0, size_t replace = 0) override {
		if (io_stream_.read_only())
			return;

		const auto total = length();
		const uint64_t insert_len = data.size();
		const uint64_t remove_len = replace;

		static constexpr size_t BUF = 64 * 1024;
		std::vector<char> buf(BUF);

		uint64_t read_pos = total;
		uint64_t write_pos = total + insert_len - remove_len;

		while (read_pos > start + remove_len) {
			size_t chunk = std::min<uint64_t>(BUF, read_pos - (start + remove_len));
			read_pos -= chunk;
			write_pos -= chunk;

			io_stream_.seek(read_pos, Beginning);
			size_t n = io_stream_.read(buf.data(), chunk);
			io_stream_.seek(write_pos, Beginning);
			io_stream_.write(buf.data(), n);
		}

		// 2) 寫入新資料
		seek(start, Beginning);
		writeBlock(data);

		// 3) 如有 replace > insert_len，砍掉多餘
		if (insert_len < remove_len) {
			truncate(total + insert_len - remove_len);
		}
	}

	void removeBlock(offset_t start = 0, size_t len = 0) override {
		if (io_stream_.read_only() || len == 0)
			return;

		const auto total = length();
		if (start + len >= total) {
			truncate(start);
			return;
		}

		static constexpr size_t BUF = 64 * 1024;
		std::vector<char> buf(BUF);

		uint64_t read_pos = start + len;
		uint64_t write_pos = start;

		while (read_pos < total) {
			size_t chunk = std::min<uint64_t>(BUF, total - read_pos);
			io_stream_.seek(read_pos, Beginning);
			size_t n = io_stream_.read(buf.data(), chunk);
			io_stream_.seek(write_pos, Beginning);
			io_stream_.write(buf.data(), n);
			read_pos += n;
			write_pos += n;
		}
		truncate(write_pos);
	}

	bool readOnly() const override {
		return io_stream_.read_only();
	}

	bool isOpen() const override {
		return io_stream_.is_open();
	}

	long long length() override {
		return static_cast<long long>(io_stream_.size());
	}

	void seek(long long offset, Position p = Beginning) override {
		int whence = (p == Beginning)
			? SEEK_SET
			: (p == Current) ? SEEK_CUR
			: SEEK_END;
		io_stream_.seek(offset, whence);
	}

	long long tell() const override {
		return static_cast<long long>(io_stream_.tell());
	}

	void truncate(long long l) override {
		if (!io_stream_.read_only())
			io_stream_.truncate(static_cast<uint64_t>(l));
	}

	FastIOStream io_stream_;
};

XAMP_METADATA_NAMESPACE_END


