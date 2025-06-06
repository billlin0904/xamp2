#include <base/dataconverter.h>

XAMP_BASE_NAMESPACE_BEGIN

AudioConvertContext::AudioConvertContext() = default;

AudioConvertContext MakeConvert(size_t convert_size) noexcept {
    AudioConvertContext context;
    context.convert_size = convert_size;
    return context;
}

AudioConverter::AudioConverter() = default;

void AudioConverter::SetFormat(uint32_t bit_per_sample, bool is_2432_format) {
	if (bit_per_sample != 16) {
		if (!is_2432_format) {
			impl_ = [](void* data, const float* buffer, const AudioConvertContext& context) noexcept {
				DataConverter<PackedFormat::INTERLEAVED, PackedFormat::INTERLEAVED>::Convert(
					static_cast<int32_t*>(data),
					buffer,
					context);
				};
		}
		else {
			impl_ = [](void* data, const float* buffer, const AudioConvertContext& context) noexcept {
				DataConverter<PackedFormat::INTERLEAVED, PackedFormat::INTERLEAVED>::ConvertToInt2432(
					static_cast<int32_t*>(data),
					buffer,
					context);
				};
		}
	}
	else {
		impl_ = [](void* data, const float* buffer, const AudioConvertContext& context) noexcept {
			DataConverter<PackedFormat::INTERLEAVED, PackedFormat::INTERLEAVED>::Convert(
				static_cast<int16_t*>(data),
				buffer,
				context);
			};
	}
}

XAMP_BASE_NAMESPACE_END
