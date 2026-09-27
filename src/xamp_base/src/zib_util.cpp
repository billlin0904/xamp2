#include <base/zib_util.h>

#include <base/dll.h>
#include <base/exception.h>
#include <base/logger.h>
#include <base/scopeguard.h>
#include <base/shared_singleton.h>
#include <base/unique_handle.h>

#include <optional>

#include <libdeflate.h>

XAMP_BASE_NAMESPACE_BEGIN

namespace {
	class LibdeflateLib final {
	public:
		XAMP_DECLARE_SINGLETON_NAME()

		LibdeflateLib();

		XAMP_DISABLE_COPY(LibdeflateLib)

	private:
		SharedLibraryHandle module_;

	public:
#ifdef XAMP_OS_WIN
		XAMP_DECLARE_DLL_NAME(libdeflate_alloc_compressor);
		XAMP_DECLARE_DLL_NAME(libdeflate_alloc_compressor_ex);
		XAMP_DECLARE_DLL_NAME(libdeflate_free_compressor);
		XAMP_DECLARE_DLL_NAME(libdeflate_alloc_decompressor);
		XAMP_DECLARE_DLL_NAME(libdeflate_alloc_decompressor_ex);
		XAMP_DECLARE_DLL_NAME(libdeflate_free_decompressor);
		XAMP_DECLARE_DLL_NAME(libdeflate_gzip_decompress);
		XAMP_DECLARE_DLL_NAME(libdeflate_zlib_decompress);
		XAMP_DECLARE_DLL_NAME(libdeflate_gzip_decompress_ex);
		XAMP_DECLARE_DLL_NAME(libdeflate_gzip_compress_bound);
		XAMP_DECLARE_DLL_NAME(libdeflate_gzip_compress);
		XAMP_DECLARE_DLL_NAME(libdeflate_deflate_compress);
		XAMP_DECLARE_DLL_NAME(libdeflate_deflate_compress_bound);
#else
		decltype(::libdeflate_alloc_compressor)* libdeflate_alloc_compressor{ nullptr };
		decltype(::libdeflate_alloc_compressor_ex)* libdeflate_alloc_compressor_ex{ nullptr };
		decltype(::libdeflate_free_compressor)* libdeflate_free_compressor{ nullptr };
		decltype(::libdeflate_alloc_decompressor)* libdeflate_alloc_decompressor{ nullptr };
		decltype(::libdeflate_alloc_decompressor_ex)* libdeflate_alloc_decompressor_ex{ nullptr };
		decltype(::libdeflate_free_decompressor)* libdeflate_free_decompressor{ nullptr };
		decltype(::libdeflate_gzip_decompress)* libdeflate_gzip_decompress{ nullptr };
		decltype(::libdeflate_zlib_decompress)* libdeflate_zlib_decompress{ nullptr };
		decltype(::libdeflate_gzip_decompress_ex)* libdeflate_gzip_decompress_ex{ nullptr };
		decltype(::libdeflate_gzip_compress_bound)* libdeflate_gzip_compress_bound{ nullptr };
		decltype(::libdeflate_gzip_compress)* libdeflate_gzip_compress{ nullptr };
		decltype(::libdeflate_deflate_compress)* libdeflate_deflate_compress{ nullptr };
		decltype(::libdeflate_deflate_compress_bound)* libdeflate_deflate_compress_bound{ nullptr };
#endif
	};

	inline LibdeflateLib::LibdeflateLib() try
#ifdef XAMP_OS_WIN
		: module_(openSharedLibrary("libdeflate"))
		, XAMP_LOAD_DLL_API(libdeflate_alloc_compressor)
		, XAMP_LOAD_DLL_API(libdeflate_alloc_compressor_ex)
		, XAMP_LOAD_DLL_API(libdeflate_free_compressor)
		, XAMP_LOAD_DLL_API(libdeflate_alloc_decompressor)
		, XAMP_LOAD_DLL_API(libdeflate_alloc_decompressor_ex)
		, XAMP_LOAD_DLL_API(libdeflate_free_decompressor)
		, XAMP_LOAD_DLL_API(libdeflate_gzip_decompress)
		, XAMP_LOAD_DLL_API(libdeflate_zlib_decompress)
		, XAMP_LOAD_DLL_API(libdeflate_gzip_decompress_ex)
		, XAMP_LOAD_DLL_API(libdeflate_gzip_compress_bound)
		, XAMP_LOAD_DLL_API(libdeflate_gzip_compress)
		, XAMP_LOAD_DLL_API(libdeflate_deflate_compress)
		, XAMP_LOAD_DLL_API(libdeflate_deflate_compress_bound) {
#else
		: libdeflate_alloc_compressor(&::libdeflate_alloc_compressor)
		, libdeflate_alloc_compressor_ex(&::libdeflate_alloc_compressor_ex)
		, libdeflate_free_compressor(&::libdeflate_free_compressor)
		, libdeflate_alloc_decompressor(&::libdeflate_alloc_decompressor)
		, libdeflate_alloc_decompressor_ex(&::libdeflate_alloc_decompressor_ex)
		, libdeflate_free_decompressor(&::libdeflate_free_decompressor)
		, libdeflate_gzip_decompress(&::libdeflate_gzip_decompress)
		, libdeflate_zlib_decompress(&::libdeflate_zlib_decompress)
		, libdeflate_gzip_decompress_ex(&::libdeflate_gzip_decompress_ex)
		, libdeflate_gzip_compress_bound(&::libdeflate_gzip_compress_bound)
		, libdeflate_gzip_compress(&::libdeflate_gzip_compress)
		, libdeflate_deflate_compress(&::libdeflate_deflate_compress)
		, libdeflate_deflate_compress_bound(&::libdeflate_deflate_compress_bound) {
#endif
	}
	catch (const Exception& e) {
		XAMP_LOG_ERROR("{}", e.getErrorMessage());
	}

#define LIBDEFLATE_LIB SharedSingleton<LibdeflateLib>::getInstance()

	libdeflate_compressor* allocCompressor(int level) {
		return LIBDEFLATE_LIB.libdeflate_alloc_compressor(level);
	}

	libdeflate_decompressor* allocDecompressor() {
		return LIBDEFLATE_LIB.libdeflate_alloc_decompressor();
	}

	void freeCompressor(libdeflate_compressor* compressor) {
		LIBDEFLATE_LIB.libdeflate_free_compressor(compressor);
	}

	void freeDecompressor(libdeflate_decompressor* decompressor) {
		LIBDEFLATE_LIB.libdeflate_free_decompressor(decompressor);
	}

	size_t gzipCompressBound(libdeflate_compressor* compressor, size_t input_size) {
		return LIBDEFLATE_LIB.libdeflate_gzip_compress_bound(compressor, input_size);
	}

	size_t gzipCompress(
		libdeflate_compressor* compressor,
		const void* in,
		size_t in_size,
		void* out,
		size_t out_size) {
		return LIBDEFLATE_LIB.libdeflate_gzip_compress(compressor, in, in_size, out, out_size);
	}

	size_t deflateCompressBound(libdeflate_compressor* compressor, size_t input_size) {
		return LIBDEFLATE_LIB.libdeflate_deflate_compress_bound(compressor, input_size);
	}

	size_t deflateCompress(
		libdeflate_compressor* compressor,
		const void* in,
		size_t in_size,
		void* out,
		size_t out_size) {
		return LIBDEFLATE_LIB.libdeflate_deflate_compress(compressor, in, in_size, out, out_size);
	}

	libdeflate_result gzipDecompress(
		libdeflate_decompressor* decompressor,
		const void* in,
		size_t in_size,
		void* out,
		size_t out_size,
		size_t* actual_out_size) {
		return LIBDEFLATE_LIB.libdeflate_gzip_decompress(
			decompressor, in, in_size, out, out_size, actual_out_size);
	}

	libdeflate_result zlibDecompress(
		libdeflate_decompressor* decompressor,
		const void* in,
		size_t in_size,
		void* out,
		size_t out_size,
		size_t* actual_out_size) {
		return LIBDEFLATE_LIB.libdeflate_zlib_decompress(
			decompressor, in, in_size, out, out_size, actual_out_size);
	}

	std::optional<uint32_t> gzipTrailerSize(const uint8_t* in_data, size_t in_size) {
		if (in_data == nullptr || in_size < 4) {
			return std::nullopt;
		}
		return static_cast<uint32_t>(in_data[in_size - 4])
			| (static_cast<uint32_t>(in_data[in_size - 3]) << 8)
			| (static_cast<uint32_t>(in_data[in_size - 2]) << 16)
			| (static_cast<uint32_t>(in_data[in_size - 1]) << 24);
	}

	bool isGzipData(const uint8_t* in_data, size_t in_size) {
		return in_data != nullptr
			&& in_size > 2
			&& in_data[0] == 0x1F
			&& in_data[1] == 0x8B;
	}

	struct LibdeflateDecompressorHandleTraits final {
		static libdeflate_decompressor* invalid() {
			return nullptr;
		}

		static void close(libdeflate_decompressor* value) {
			freeDecompressor(value);
		}
	};

	struct LibdeflateCompressorHandleTraits final {
		static libdeflate_compressor* invalid() {
			return nullptr;
		}

		static void close(libdeflate_compressor* value) {
			freeCompressor(value);
		}
	};

	using LibdeflateDecompressorHandle = UniqueHandle<libdeflate_decompressor*, LibdeflateDecompressorHandleTraits>;
	using LibdeflateCompressorHandle = UniqueHandle<libdeflate_compressor*, LibdeflateCompressorHandleTraits>;
}

void loadLibdeflate() {
	SharedSingleton<LibdeflateLib>::getInstance();
}

std::expected<std::vector<uint8_t>, GzipDecompressError> gzipCompress(
	const uint8_t* in_data,
	size_t in_size,
	CompressType compress_type) {
	if (in_data == nullptr || in_size == 0) {
		return std::unexpected(GzipDecompressError::GZIP_COMPRESS_ERROR_EMPTY_INPUT);
	}

	constexpr int kDefaultLevel = 6;

	LibdeflateCompressorHandle handle(allocCompressor(kDefaultLevel));
	if (!handle) {
		return std::unexpected(GzipDecompressError::GZIP_COMPRESS_ERROR_UNKNOWN);
	}

	size_t bound = 0;
	size_t actual = 0;
	if (compress_type != CompressType::COMPRESS_DEFLATE) {
		bound = gzipCompressBound(handle.get(), in_size);
	}
	else {
		bound = deflateCompressBound(handle.get(), in_size);
	}

	std::vector<uint8_t> out(bound);
	if (compress_type != CompressType::COMPRESS_DEFLATE) {
		actual = gzipCompress(handle.get(), in_data, in_size, out.data(), out.size());
	}
	else {
		actual = deflateCompress(handle.get(), in_data, in_size, out.data(), out.size());
	}

	if (actual == 0) {
		return std::unexpected(GzipDecompressError::GZIP_COMPRESS_ERROR_UNKNOWN);
	}
	out.resize(actual);
	return out;
}

std::expected<std::vector<uint8_t>, GzipDecompressError> gzipCompress(
	const std::vector<uint8_t>& data,
	CompressType compress_type) {
	return gzipCompress(data.data(), data.size(), compress_type);
}

std::expected<std::vector<uint8_t>, GzipDecompressError> gzipDecompressBytes(
	const uint8_t* in_data,
	size_t in_size) {
	if (in_data == nullptr || in_size == 0) {
		return std::unexpected(GzipDecompressError::GZIP_COMPRESS_ERROR_EMPTY_INPUT);
	}

	LibdeflateDecompressorHandle handle(allocDecompressor());
	if (!handle) {
		return std::unexpected(GzipDecompressError::GZIP_COMPRESS_ERROR_UNKNOWN);
	}

	constexpr size_t kDefaultBufferSize = 40960;
	size_t guest_decompress_size = gzipTrailerSize(in_data, in_size).value_or(in_size * 3);
	if (guest_decompress_size > kDefaultBufferSize) {
		guest_decompress_size = kDefaultBufferSize;
	}

	std::vector<uint8_t> out;
	size_t actual = 0;
	libdeflate_result res;
	while (true) {
		out.resize(guest_decompress_size);

		if (isGzipData(in_data, in_size)) {
			res = gzipDecompress(
				handle.get(),
				in_data,
				in_size,
				out.data(),
				guest_decompress_size,
				&actual);
		}
		else {
			res = zlibDecompress(
				handle.get(),
				in_data,
				in_size,
				out.data(),
				guest_decompress_size,
				&actual);
		}

		if (res == LIBDEFLATE_SUCCESS) {
			out.resize(actual);
			break;
		}

		if (res == LIBDEFLATE_INSUFFICIENT_SPACE) {
			guest_decompress_size *= 2;
			continue;
		}

		return std::unexpected(GzipDecompressError::GZIP_COMPRESS_ERROR_BAD_DATA);
	}

	return out;
}

std::expected<std::vector<uint8_t>, GzipDecompressError> gzipDecompressBytes(
	const std::vector<uint8_t>& data) {
	return gzipDecompressBytes(data.data(), data.size());
}

std::expected<std::string, GzipDecompressError> gzipDecompress(
	const uint8_t* in_data,
	size_t in_size) {
	auto decompressed = gzipDecompressBytes(in_data, in_size);
	if (!decompressed) {
		return std::unexpected(decompressed.error());
	}

	const auto& bytes = decompressed.value();
	return std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}

std::expected<std::string, GzipDecompressError> gzipDecompress(
	const std::vector<uint8_t>& data) {
	return gzipDecompress(data.data(), data.size());
}

XAMP_BASE_NAMESPACE_END
