#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>

#include <base/fs.h>
#include <base/base.h>

XAMP_BASE_NAMESPACE_BEGIN

// Read-only raw NTFS stream. Parses the unnamed $DATA attributes from MFT
// records, maps logical offsets through data runs and reads the volume itself.
// Resident bytes, sparse runs and uninitialized tails are handled by ntfs.h.
// Keeps a read handle denying writes/deletion and protects clusters from moves.
// Construction throws on unsupported platforms, non-NTFS volumes or denied
// volume/cluster-protection access. Compressed, encrypted and reparse files
// require ordinary path-based fallback. No file contents are read by path/ID.
// A stream is not thread-safe; use one instance per metadata reader.
class XAMP_BASE_API NtfsFileStream final {
public:
	explicit NtfsFileStream(const Path& path);

	~NtfsFileStream();

	XAMP_DISABLE_COPY_AND_MOVE(NtfsFileStream)

	size_t read(void* buffer, size_t length);

	void seek(int64_t offset, int origin);

	[[nodiscard]] uint64_t tell() const;

	[[nodiscard]] uint64_t size() const;

	[[nodiscard]] uint64_t fileReference() const;

private:
	class Impl;
	std::unique_ptr<Impl> impl_;
};

XAMP_BASE_NAMESPACE_END
