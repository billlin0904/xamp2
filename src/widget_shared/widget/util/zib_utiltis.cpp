#include <widget/util/zib_utiltis.h>
#include <widget/widget_shared.h>
#include <base/dll.h>
#include <base/scopeguard.h>

#include <fstream>

#include <zlib.h>
#include <contrib/minizip/unzip.h>

namespace {
    inline constexpr auto kReadZipBufferSize = 4096;

    struct UzFileHandleTraits final {
        static unzFile invalid() noexcept {
            return nullptr;
        }

        static void close(unzFile value) noexcept {
            unzClose(value);
        }
    };

    voidpf fopen64_file_func(voidpf opaque, const void* filename, int mode) {
        FILE* file = nullptr;
        const wchar_t* mode_fopen = nullptr;

        if ((mode & ZLIB_FILEFUNC_MODE_READWRITEFILTER) == ZLIB_FILEFUNC_MODE_READ)
            mode_fopen = L"rb";
        else
            if (mode & ZLIB_FILEFUNC_MODE_EXISTING)
                mode_fopen = L"r+b";
            else
                if (mode & ZLIB_FILEFUNC_MODE_CREATE)
                    mode_fopen = L"wb";

        if ((filename != nullptr) && (mode_fopen != nullptr))
            _wfopen_s(&file, static_cast<const wchar_t*>(filename), mode_fopen);
        return file;
    }


    uLong fread_file_func(voidpf opaque, voidpf stream, void* buf, uLong size) {
        uLong ret;
        ret = static_cast<uLong>(fread(buf, 1, (size_t)size, static_cast<FILE*>(stream)));
        return ret;
    }

    uLong fwrite_file_func(voidpf opaque, voidpf stream, const void* buf, uLong size) {
        uLong ret;
        ret = static_cast<uLong>(fwrite(buf, 1, (size_t)size, static_cast<FILE*>(stream)));
        return ret;
    }

    ZPOS64_T ftell64_file_func(voidpf opaque, voidpf stream) {
        ZPOS64_T ret;
        ret = _ftelli64(static_cast<FILE*>(stream));
        return ret;
    }

    long fseek64_file_func(voidpf opaque, voidpf stream, ZPOS64_T offset, int origin) {
        int fseek_origin = 0;
        long ret;

        switch (origin)
        {
        case ZLIB_FILEFUNC_SEEK_CUR:
            fseek_origin = SEEK_CUR;
            break;
        case ZLIB_FILEFUNC_SEEK_END:
            fseek_origin = SEEK_END;
            break;
        case ZLIB_FILEFUNC_SEEK_SET:
            fseek_origin = SEEK_SET;
            break;
        default: return -1;
        }

        ret = 0;

        if (_fseeki64(static_cast<FILE*>(stream), offset, fseek_origin) != 0)
            ret = -1;

        return ret;
    }

    int fclose_file_func(voidpf opaque, voidpf stream) {
        int ret;
        ret = fclose(static_cast<FILE*>(stream));
        return ret;
    }

    int ferror_file_func(voidpf opaque, voidpf stream) {
        int ret;
        ret = ferror(static_cast<FILE*>(stream));
        return ret;
    }
}

XAMP_PIMPL_IMPL(ZipFileReader)

class ZipFileReader::ZipFileReaderImpl {
public:
    ZipFileReaderImpl() = default;
    
    std::vector<std::wstring> OpenFile(const std::wstring& file) {
        zlib_filefunc64_def func_def{};
        func_def.zopen64_file = fopen64_file_func;
        func_def.zread_file = fread_file_func;
        func_def.zwrite_file = fwrite_file_func;
        func_def.ztell64_file = ftell64_file_func;
        func_def.zseek64_file = fseek64_file_func;
        func_def.zclose_file = fclose_file_func;
        func_def.zerror_file = ferror_file_func;
        func_def.opaque = nullptr;

        handle_.reset(::unzOpen2_64(file.c_str(), &func_def));

        std::vector<std::wstring> track_infos;

        unz_global_info zip_file_info{};
        if (::unzGetGlobalInfo(handle_.get(), &zip_file_info) != UNZ_OK) {
            throw Exception();
        }

        for (uint32_t i = 0; i < zip_file_info.number_entry; i++) {
            char file_name[MAX_PATH]{};
            unz_file_info file_info{};
            const auto ret = ::unzGetCurrentFileInfo(handle_.get(),
                                                   &file_info, 
                                                   file_name, 
                                                   MAX_PATH, 
                                                   nullptr, 
                                                   0,
                                                   nullptr,
                                                   0);
            if (ret != UNZ_OK) {
                throw Exception();
            }
            
            const size_t filename_length = strlen(file_name);
            if (file_name[filename_length - 1] != '/') {
                XAMP_LOG_DEBUG("File {}", file_name);
                Path filename_path(String::ToString(file_name));
                auto ext = filename_path.extension().string();
                if (GetSupportFileExtensions().contains(ext)) {
                    track_infos.push_back(ExtractCurrentFile());
                }                              
            }
            else {
                XAMP_LOG_DEBUG("Directory {}", file_name);
            }
            unzGoToNextFile(handle_.get());
        }

        return track_infos;
    }

    std::wstring ExtractCurrentFile() const {
        if (::unzOpenCurrentFile(handle_.get()) != UNZ_OK) {
            throw Exception();
        }

        const auto temp_file_path = GetTempFileNamePath();

        XAMP_ON_SCOPE_FAIL(
            ::unzCloseCurrentFile(handle_.get());            
        );
        
        std::ofstream out(temp_file_path, std::ios::binary);        
        if (!out.is_open()) {            
            throw Exception();
        }
        
        int error = UNZ_OK;
        do {
            char output[kReadZipBufferSize]{};
            
            error = ::unzReadCurrentFile(handle_.get(), output, kReadZipBufferSize);

            if (error < 0) {
                throw Exception();
            }
            out.write(output, error);
        } while (error > 0);

        out.close();
        return temp_file_path.wstring();
    }

    using UzFileHandle = UniqueHandle<unzFile, UzFileHandleTraits>;
    UzFileHandle handle_;
};

ZipFileReader::ZipFileReader()
    : impl_(MakeAlign<ZipFileReaderImpl>()) {
}

std::vector<std::wstring> ZipFileReader::openFile(const std::wstring& file) {
    return impl_->OpenFile(file);
}

QByteArray gzipDecompress(const QByteArray& data) {
#define XAMP_inflateInit2(strm, windowBits) \
          ::inflateInit2_((strm), (windowBits), ZLIB_VERSION, \
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
        ::inflateEnd(&zlib_stream);
        );

    do {
        char output[kReadZipBufferSize]{ };
        zlib_stream.avail_out = kReadZipBufferSize;
        zlib_stream.next_out = reinterpret_cast<Bytef*>(output);

        ret = ::inflate(&zlib_stream, Z_NO_FLUSH);
        Q_ASSERT(ret != Z_STREAM_ERROR);  // state not clobbered

        switch (ret) {
        case Z_NEED_DICT:
            ret = Z_DATA_ERROR;     // and fall through
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
            throw std::exception("Date error");
        }

        result.append(output, kReadZipBufferSize - zlib_stream.avail_out);
    } while (zlib_stream.avail_out == 0);

    return result;
}
