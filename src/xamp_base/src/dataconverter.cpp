#include <base/dataconverter.h>

XAMP_BASE_NAMESPACE_BEGIN

AudioConvertContext::AudioConvertContext() {
    in_offset.fill(0);
    out_offset.fill(0);
}

AudioConvertContext MakeConvert(AudioFormat const& in_format, AudioFormat const& out_format, size_t convert_size) noexcept {
    AudioConvertContext context;

    context.in_jump = in_format.GetChannels();
    context.out_jump = out_format.GetChannels();

    context.input_format = in_format;
    context.output_format = out_format;

    context.convert_size = convert_size;

    context.in_offset.fill(0);
    context.out_offset.fill(0);

    if (in_format.GetPackedFormat() == out_format.GetPackedFormat()) {
        if (in_format.GetPackedFormat() == PackedFormat::INTERLEAVED) {
            for (size_t k = 0, i = 0; k < in_format.GetChannels(); ++k, ++i) {
                context.in_offset[i] = k;
                context.out_offset[i] = k;
            }
        }
        else {
            for (size_t k = 0, i = 0; k < in_format.GetChannels(); ++k, ++i) {
                context.in_offset[i] = k * convert_size;
                context.out_offset[i] = k * convert_size;
            }
            context.in_jump = 1;
            context.out_jump = 1;
        }
    }
    else {
        for (size_t k = 0, i = 0; k < in_format.GetChannels(); ++k, ++i) {
            switch (in_format.GetPackedFormat()) {
            case PackedFormat::INTERLEAVED:
                context.in_offset[i] = k;
                break;
            case PackedFormat::PLANAR:
                context.in_offset[i] = k * convert_size;
                context.in_jump = 1;
                break;
            }
            switch (out_format.GetPackedFormat()) {
            case PackedFormat::INTERLEAVED:
                context.out_offset[i] = k;
                break;
            case PackedFormat::PLANAR:
                context.out_offset[i] = k * convert_size;
                context.out_jump = 1;
                break;
            }
        }
    }
    return context;
}

XAMP_BASE_NAMESPACE_END
