//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <metadata/taglib.h>
#include <base/fs.h>
#include <base/stl.h>
#include <base/fastiostream.h>
#include <base/memory.h>

#include <limits>

XAMP_METADATA_NAMESPACE_BEGIN

class TaglibIOStream final : public TagLib::IOStream {
public:
	TaglibIOStream() = default;

	virtual ~TaglibIOStream() override = default;

	explicit TaglibIOStream(const Path& path, FastIOStream::Mode mode = FastIOStream::Mode::ReadWriteOnlyExisting)
		: io_stream_(path, mode) {
	}

	void open(const Path& path, FastIOStream::Mode mode = FastIOStream::Mode::ReadWriteOnlyExisting) {
		io_stream_.open(path, mode);
	}

	void close() {
		io_stream_.close();
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
		if (io_stream_.read_only() || start < 0)
			return;

		const auto total = static_cast<uint64_t>(length());
		const auto start_pos = static_cast<uint64_t>(start);
		if (start_pos > total)
			return;

		const uint64_t insert_len = data.size();
		const uint64_t remove_len = std::min<uint64_t>(replace, total - start_pos);
		const uint64_t tail_pos = start_pos + remove_len;
		const uint64_t write_data_end = start_pos + insert_len;
		const uint64_t new_total = total - remove_len + insert_len;

		static constexpr size_t BUF = 64 * 1024;
		std::vector<char> buf(BUF);

		auto seek_absolute = [this](uint64_t pos) {
			if (pos > static_cast<uint64_t>((std::numeric_limits<int64_t>::max)()))
				throw PlatformException();
			io_stream_.seek(static_cast<int64_t>(pos), SEEK_SET);
			};

		auto write_all = [this](const char* data, size_t len) {
			size_t total_written = 0;
			while (total_written < len) {
				const auto written = io_stream_.write(data + total_written, len - total_written);
				if (written == 0)
					throw PlatformException();
				total_written += written;
			}
			};

		if (insert_len > remove_len) {
			uint64_t read_pos = total;
			uint64_t write_pos = new_total;
			while (read_pos > tail_pos) {
				const size_t chunk = static_cast<size_t>(std::min<uint64_t>(BUF, read_pos - tail_pos));
				read_pos -= chunk;
				write_pos -= chunk;

				seek_absolute(read_pos);
				const size_t n = io_stream_.read(buf.data(), chunk);
				if (n == 0)
					throw PlatformException();
				seek_absolute(write_pos);
				write_all(buf.data(), n);
			}
		}
		else if (insert_len < remove_len) {
			uint64_t read_pos = tail_pos;
			uint64_t write_pos = write_data_end;
			while (read_pos < total) {
				const size_t chunk = static_cast<size_t>(std::min<uint64_t>(BUF, total - read_pos));
				seek_absolute(read_pos);
				const size_t n = io_stream_.read(buf.data(), chunk);
				if (n == 0)
					throw PlatformException();
				seek_absolute(write_pos);
				write_all(buf.data(), n);
				read_pos += n;
				write_pos += n;
			}
		}

		seek(start, Beginning);
		writeBlock(data);

		if (insert_len < remove_len) {
			truncate(static_cast<long long>(new_total));
		}
	}

	void removeBlock(offset_t start = 0, size_t len = 0) override {
		if (io_stream_.read_only() || len == 0 || start < 0)
			return;

		const auto total = static_cast<uint64_t>(length());
		const auto start_pos = static_cast<uint64_t>(start);
		if (start_pos >= total)
			return;

		const auto remove_len = std::min<uint64_t>(len, total - start_pos);
		if (start_pos + remove_len >= total) {
			truncate(start);
			return;
		}

		static constexpr size_t BUF = 64 * 1024;
		std::vector<char> buf(BUF);

		auto seek_absolute = [this](uint64_t pos) {
			if (pos > static_cast<uint64_t>((std::numeric_limits<int64_t>::max)()))
				throw PlatformException();
			io_stream_.seek(static_cast<int64_t>(pos), SEEK_SET);
			};

		auto write_all = [this](const char* data, size_t len) {
			size_t total_written = 0;
			while (total_written < len) {
				const auto written = io_stream_.write(data + total_written, len - total_written);
				if (written == 0)
					throw PlatformException();
				total_written += written;
			}
			};

		uint64_t read_pos = start_pos + remove_len;
		uint64_t write_pos = start_pos;

		while (read_pos < total) {
			const size_t chunk = static_cast<size_t>(std::min<uint64_t>(BUF, total - read_pos));
			seek_absolute(read_pos);
			const size_t n = io_stream_.read(buf.data(), chunk);
			if (n == 0)
				throw PlatformException();
			seek_absolute(write_pos);
			write_all(buf.data(), n);
			read_pos += n;
			write_pos += n;
		}
		truncate(static_cast<long long>(write_pos));
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


