#include <base/dataconverter.h>

XAMP_BASE_NAMESPACE_BEGIN

AudioConvertContext::AudioConvertContext() = default;

AudioConvertContext MakeConvert(size_t convert_size) noexcept {
    AudioConvertContext context;
    context.convert_size = convert_size;
    return context;
}

XAMP_BASE_NAMESPACE_END
