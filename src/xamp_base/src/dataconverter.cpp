#include <base/dataconverter.h>

namespace xamp::base {

AudioConvertContext::AudioConvertContext() {
	in_offset.fill(0);
	out_offset.fill(0);
}

AudioConvertContext MakeConvert(const AudioFormat& in_format, const AudioFormat& out_format, size_t convert_size) noexcept {
	AudioConvertContext context;

	context.in_jump = in_format.GetChannels();
	context.out_jump = out_format.GetChannels();

	context.input_format = in_format;
	context.output_format = out_format;

	context.convert_size = convert_size;

	context.in_offset.fill(0);
	context.out_offset.fill(0);

	if (in_format.GetInterleavedFormat() == out_format.GetInterleavedFormat()) {
		if (in_format.GetInterleavedFormat() == InterleavedFormat::INTERLEAVED) {
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
			switch (in_format.GetInterleavedFormat()) {
			case InterleavedFormat::INTERLEAVED:
				context.in_offset[i] = k;
				break;
			case InterleavedFormat::DEINTERLEAVED:
				context.in_offset[i] = k * convert_size;
				context.in_jump = 1;
				break;
			}
			switch (out_format.GetInterleavedFormat()) {
			case InterleavedFormat::INTERLEAVED:
				context.out_offset[i] = k;
				break;
			case InterleavedFormat::DEINTERLEAVED:
				context.out_offset[i] = k * convert_size;
				context.out_jump = 1;
				break;
			}
		}
	}
	return context;
}

}

