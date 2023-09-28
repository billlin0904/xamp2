#include <widget/zib_utiltis.h>
#include <widget/widget_shared.h>
#include <base/dll.h>
#include <base/scopeguard.h>
#include <zlib.h>

class ZLibLib final {
public:
    ZLibLib();

private:
    SharedLibraryHandle module_;

public:
    XAMP_DECLARE_DLL_NAME(inflateInit2_);
    XAMP_DECLARE_DLL_NAME(inflate);
    XAMP_DECLARE_DLL_NAME(inflateEnd);
};

ZLibLib::ZLibLib()
    : module_(OpenSharedLibrary("zlib1"))
    , XAMP_LOAD_DLL_API(inflateInit2_)
    , XAMP_LOAD_DLL_API(inflate)
    , XAMP_LOAD_DLL_API(inflateEnd) {
}

#define ZLIB_DLL Singleton<ZLibLib>::GetInstance()

bool IsLoadZib() {
    try {
        Singleton<ZLibLib>::GetInstance();
        return true;
    } catch (...) {
        return false;
    }
}

QByteArray GzipDecompress(const QByteArray& data) {
#define XAMP_inflateInit2(strm, windowBits) \
          ZLIB_DLL.inflateInit2_((strm), (windowBits), ZLIB_VERSION, \
                        (int)sizeof(z_stream))
    QByteArray result;
    z_stream zlib_stream;

    zlib_stream.zalloc = nullptr;
    zlib_stream.zfree = nullptr;
    zlib_stream.opaque = nullptr;
    zlib_stream.avail_in = data.size();
    zlib_stream.next_in = (Bytef*)data.data();

    auto ret = XAMP_inflateInit2(&zlib_stream, 15 + 32); // gzip decoding
    if (ret != Z_OK) {
        return result;
    }

    XAMP_ON_SCOPE_EXIT(
        ZLIB_DLL.inflateEnd(&zlib_stream);
        );

    static constexpr auto kBufferSize = 1024;

    do {
        char output[kBufferSize]{ };
        zlib_stream.avail_out = kBufferSize;
        zlib_stream.next_out = reinterpret_cast<Bytef*>(output);

        ret = ZLIB_DLL.inflate(&zlib_stream, Z_NO_FLUSH);
        Q_ASSERT(ret != Z_STREAM_ERROR);  // state not clobbered

        switch (ret) {
        case Z_NEED_DICT:
            ret = Z_DATA_ERROR;     // and fall through
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
            throw std::exception("Date error");
        }

        result.append(output, kBufferSize - zlib_stream.avail_out);
    } while (zlib_stream.avail_out == 0);

    return result;
}
