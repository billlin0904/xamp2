// This is an open source non-commercial project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

extern "C" {
#include <libavutil/error.h>
}

#include <stream/avexception.h>

namespace xamp::stream {

AvException::AvException(int32_t error)
    : Exception(Errors::XAMP_ERROR_LIBRARY_SPEC_ERROR) {
    char buf[256]{};
    av_strerror(error, buf, sizeof(buf) - 1);
    message_.assign(buf);
}

AvException::~AvException() = default;

}
