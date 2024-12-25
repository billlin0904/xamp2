#include <base/dataconverter.h>

XAMP_BASE_NAMESPACE_BEGIN

AudioConvertContext::AudioConvertContext() = default;

AudioConvertContext MakeConvert(AudioFormat const& in_format, AudioFormat const& out_format, size_t convert_size) noexcept {
    AudioConvertContext context;
    context.input_format = in_format;
    context.output_format = out_format;
    context.convert_size = convert_size;
    return context;
}

XAMP_BASE_NAMESPACE_END
