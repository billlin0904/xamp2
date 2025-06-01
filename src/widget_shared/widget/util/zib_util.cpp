#include <widget/util/zib_util.h>
#include <widget/widget_shared.h>
#include <base/dll.h>
#include <base/scopeguard.h>

#include <fstream>
#include <QtEndian>
#include <QScopeGuard>

#define USE_ZLIB 0

#if USE_ZLIB

#include <zlib.h>
#include <contrib/minizip/unzip.h>

namespace {
#define XAMP_inflateInit2(strm, windowBits) \
          ::inflateInit2_((strm), (windowBits), ZLIB_VERSION, \
                        (int)sizeof(z_stream))

#define XAMP_deflateInit2(strm, level, windowBits) \
        ::deflateInit2_((strm), (level), Z_DEFLATED, (windowBits), 8, Z_DEFAULT_STRATEGY, ZLIB_VERSION, sizeof(z_stream))

    constexpr auto kZipBufferSize = 4096;
}

std::expected<QByteArray, GzipDecompressError> gzipCompress(const QByteArray& data) {
    if (data.isEmpty())
        return std::unexpected(GzipDecompressError::GZIP_COMPRESS_ERROR_EMPTY_INPUT);

    z_stream stream{};
    stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(data.data()));
    stream.avail_in = data.size();

    int level = Z_DEFAULT_COMPRESSION;

    // windowBits = 15 | 16 -> 15+16=31 表示產生 gzip 包裝
    if (XAMP_deflateInit2(&stream, level, 31) != Z_OK)
        return std::unexpected(GzipDecompressError::GZIP_COMPRESS_ERROR_UNKNOWN);

    XAMP_ON_SCOPE_EXIT(
        ::deflateEnd(&stream);
    );

    QByteArray out;
    int ret = 0;
    char buffer[kZipBufferSize]{ };

    do {
        stream.avail_out = kZipBufferSize;
        stream.next_out = reinterpret_cast<Bytef*>(buffer);

        ret = ::deflate(&stream, stream.avail_in ? Z_NO_FLUSH : Z_FINISH);
        if (ret == Z_STREAM_ERROR)
            return std::unexpected(GzipDecompressError::GZIP_COMPRESS_ERROR_UNKNOWN);

        out.append(buffer, kZipBufferSize - stream.avail_out);
    } while (ret != Z_STREAM_END);

    return out;
}

std::expected<QByteArray, GzipDecompressError> gzipDecompress(const QByteArray& data) {
    QByteArray result;
    z_stream zlib_stream;

    zlib_stream.zalloc = nullptr;
    zlib_stream.zfree = nullptr;
    zlib_stream.opaque = nullptr;
    zlib_stream.avail_in = data.size();
    zlib_stream.next_in = (Bytef*)data.data();

    auto ret = XAMP_inflateInit2(&zlib_stream, 15 + 32); // gzip decoding
    if (ret != Z_OK) {
        return std::unexpected(GzipDecompressError::GZIP_COMPRESS_ERROR_UNKNOWN);
    }

    XAMP_ON_SCOPE_EXIT(
        ::inflateEnd(&zlib_stream);
    );
    
    do {
        char output[kZipBufferSize]{ };
        zlib_stream.avail_out = kZipBufferSize;
        zlib_stream.next_out = reinterpret_cast<Bytef*>(output);

        ret = ::inflate(&zlib_stream, Z_NO_FLUSH);
        Q_ASSERT(ret != Z_STREAM_ERROR);  // state not clobbered

        switch (ret) {
        case Z_NEED_DICT:
            ret = Z_DATA_ERROR;     // and fall through            
        case Z_DATA_ERROR:                        
        case Z_MEM_ERROR:
            return std::unexpected(GzipDecompressError::GZIP_COMPRESS_ERROR_UNKNOWN);
        }

        result.append(output, kZipBufferSize - zlib_stream.avail_out);
    } while (zlib_stream.avail_out == 0);

    return result;
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

#else
#include <libdeflate.h>

namespace {
    class LibdeflateLib final {
    public:
        LibdeflateLib();

        XAMP_DISABLE_COPY(LibdeflateLib)

    private:
        SharedLibraryHandle module_;

    public:
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
    };

    inline LibdeflateLib::LibdeflateLib() try
        : module_(OpenSharedLibrary("libdeflate"))
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
        }
        catch (const Exception& e) {
            XAMP_LOG_ERROR("{}", e.GetErrorMessage());
        }

#define LIBDEFLATE_LIB SharedSingleton<LibdeflateLib>::GetInstance()

        std::optional<quint32> gzipTrailerSize(const QByteArray& data) {
            if (data.size() < 4)
                return std::nullopt;
            quint32 isize = 0;
            memcpy(&isize, data.constData() + data.size() - 4, 4);
            return qFromLittleEndian(isize);
        }

        struct LibdeflateDecompressorHandleTraits final {
            static libdeflate_decompressor* invalid() noexcept {
                return nullptr;
            }

            static void close(libdeflate_decompressor* value) noexcept {
                LIBDEFLATE_LIB.libdeflate_free_decompressor(value);
            }
        };

        struct LibdeflateCompressorHandleTraits final {
            static libdeflate_compressor* invalid() noexcept {
                return nullptr;
            }

            static void close(libdeflate_compressor* value) noexcept {
                LIBDEFLATE_LIB.libdeflate_free_compressor(value);
            }
        };

        using LibdeflateDecompressorHandle = UniqueHandle<libdeflate_decompressor*, LibdeflateDecompressorHandleTraits>;
        using LibdeflateCompressorHandle = UniqueHandle<libdeflate_compressor*, LibdeflateCompressorHandleTraits>;
}

std::expected<QByteArray, GzipDecompressError> gzipCompress(const QByteArray& data, CompressType compress_type) {
    if (data.isEmpty()) {
        return std::unexpected(GzipDecompressError::GZIP_COMPRESS_ERROR_EMPTY_INPUT);
    }

    // 預設壓縮等級： 0~12 (libdeflate v1.20)；6 為 zlib 預設的平衡值
    constexpr int kDefaultLevel = 6;

    LibdeflateCompressorHandle handle(LIBDEFLATE_LIB.libdeflate_alloc_compressor(kDefaultLevel)); // 6: Default
    if (!handle) {
        return std::unexpected(GzipDecompressError::GZIP_COMPRESS_ERROR_UNKNOWN);
    }

    QByteArray out;
    size_t bound;
    size_t actual;

    if (compress_type != CompressType::COMPRESS_DEFLATE) {
        bound = LIBDEFLATE_LIB.libdeflate_gzip_compress_bound(handle.get(), data.size());
        out.resize(static_cast<int>(bound));
        actual = LIBDEFLATE_LIB.libdeflate_gzip_compress(
            handle.get(),
            data.constData(), data.size(),
            out.data(),
            bound);
    }
    else {
        bound = LIBDEFLATE_LIB.libdeflate_deflate_compress_bound(handle.get(), data.size());
        out.resize(static_cast<int>(bound));
        actual = LIBDEFLATE_LIB.libdeflate_deflate_compress(
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

    LibdeflateDecompressorHandle handle(LIBDEFLATE_LIB.libdeflate_alloc_decompressor());
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
            res = LIBDEFLATE_LIB.libdeflate_gzip_decompress(
                handle.get(),
                data.constData(), data.size(),
                out.data(), guestDecompressSize,
                &actual);
        }
        else {
            res = LIBDEFLATE_LIB.libdeflate_zlib_decompress(
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
#endif