#include <widget/util/zib_util.h>
#include <widget/widget_shared.h>
#include <base/dll.h>
#include <base/scopeguard.h>

#include <fstream>
#include <QtEndian>
#include <QScopeGuard>

#include <libdeflate.h>

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
        decltype(::libdeflate_alloc_compressor)*      libdeflate_alloc_compressor{ nullptr };
		decltype(::libdeflate_alloc_compressor_ex)*   libdeflate_alloc_compressor_ex{ nullptr };
		decltype(::libdeflate_free_compressor)*       libdeflate_free_compressor{ nullptr };
		decltype(::libdeflate_alloc_decompressor)*    libdeflate_alloc_decompressor{ nullptr };
        decltype(::libdeflate_alloc_decompressor_ex)* libdeflate_alloc_decompressor_ex{ nullptr };
        decltype(::libdeflate_free_decompressor)*     libdeflate_free_decompressor{ nullptr };
        decltype(::libdeflate_gzip_decompress)*       libdeflate_gzip_decompress{ nullptr };
        decltype(::libdeflate_zlib_decompress)*       libdeflate_zlib_decompress{ nullptr };
        decltype(::libdeflate_gzip_decompress_ex)*    libdeflate_gzip_decompress_ex{ nullptr };
        decltype(::libdeflate_gzip_compress_bound)*   libdeflate_gzip_compress_bound{ nullptr };
        decltype(::libdeflate_gzip_compress)*         libdeflate_gzip_compress{ nullptr };
		decltype(::libdeflate_deflate_compress)*      libdeflate_deflate_compress{ nullptr };
        decltype(::libdeflate_deflate_compress)*      libdeflate_deflate_compress_bound{ nullptr };
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

    std::optional<quint32> gzipTrailerSize(const QByteArray& data) {
        if (data.size() < 4)
            return std::nullopt;
        quint32 isize = 0;
        memcpy(&isize, data.constData() + data.size() - 4, 4);
        return qFromLittleEndian(isize);
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

std::expected<QByteArray, GzipDecompressError> gzipCompress(const QByteArray& data, CompressType compress_type) {
    if (data.isEmpty()) {
        return std::unexpected(GzipDecompressError::GZIP_COMPRESS_ERROR_EMPTY_INPUT);
    }

    // 預設壓縮等級： 0~12 (libdeflate v1.20)；6 為 zlib 預設的平衡值
    constexpr int kDefaultLevel = 6;

    LibdeflateCompressorHandle handle(allocCompressor(kDefaultLevel)); // 6: Default
    if (!handle) {
        return std::unexpected(GzipDecompressError::GZIP_COMPRESS_ERROR_UNKNOWN);
    }

    QByteArray out;
    size_t bound;
    size_t actual;

    if (compress_type != CompressType::COMPRESS_DEFLATE) {
        bound = gzipCompressBound(handle.get(), data.size());
        out.resize(static_cast<int>(bound));
        actual = gzipCompress(
            handle.get(),
            data.constData(), data.size(),
            out.data(),
            bound);
    }
    else {
        bound = deflateCompressBound(handle.get(), data.size());
        out.resize(static_cast<int>(bound));
        actual = deflateCompress(
            handle.get(),
            data.constData(), data.size(),
            out.data(),
            bound);
    }

    if (actual == 0) {
        return std::unexpected(GzipDecompressError::GZIP_COMPRESS_ERROR_UNKNOWN);
    }
    out.truncate(static_cast<int>(actual));
    return out;
}

std::expected<QByteArray, GzipDecompressError> gzipDecompress(const QByteArray& data) {
    if (data.isEmpty()) {
        return std::unexpected(GzipDecompressError::GZIP_COMPRESS_ERROR_EMPTY_INPUT);
    }

    LibdeflateDecompressorHandle handle(allocDecompressor());
    if (!handle) {
        return std::unexpected(GzipDecompressError::GZIP_COMPRESS_ERROR_UNKNOWN);
    }

    const size_t kDefaultBufferSize = 40960;
    size_t guestDecompressSize = gzipTrailerSize(data).value_or(data.size() * 3);

    if (guestDecompressSize > kDefaultBufferSize)
        guestDecompressSize = kDefaultBufferSize;

    QByteArray out;
    size_t actual = 0;
    libdeflate_result res;
    while (true) {
        out.resize(static_cast<int>(guestDecompressSize));

        auto isGzip = (data.size() > 2)
            && ((uint8_t)data[0] == 0x1F) 
            && ((uint8_t)data[1] == 0x8B);

        if (isGzip) {
            res = gzipDecompress(
                handle.get(),
                data.constData(), data.size(),
                out.data(), guestDecompressSize,
                &actual);
        }
        else {
            res = zlibDecompress(
                handle.get(),
                data.constData(), data.size(),
                out.data(), guestDecompressSize,
                &actual);
        }        

        if (res == LIBDEFLATE_SUCCESS) {
            out.resize(static_cast<int>(actual));
            break;
        }

        if (res == LIBDEFLATE_INSUFFICIENT_SPACE) {
            guestDecompressSize *= 2;
            continue;
        }

        out.clear();
        return std::unexpected(GzipDecompressError::GZIP_COMPRESS_ERROR_BAD_DATA);
    }

    return out;
}

std::expected<std::string, GzipDecompressError> gzipDecompress(const uint8_t* in_data, size_t in_size) {
    std::string out_data;
    QByteArray data(reinterpret_cast<const char*>(in_data), static_cast<int>(in_size));
    auto decompressed = gzipDecompress(data);
    if (!decompressed) {
        return std::unexpected(decompressed.error());
    }
    return decompressed.value().toStdString();  
}

