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
