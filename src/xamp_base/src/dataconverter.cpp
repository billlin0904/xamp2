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
			// TODO: Add 16/24/32 bit conversion support.
			switch (bit_per_sample) {
			case 24:
				// TODO: bit-perfect implementation for int24
				impl_ = [](void* data, const void* buffer, const AudioConvertContext& context) noexcept {
					DataConverter<PackedFormat::INTERLEAVED, PackedFormat::INTERLEAVED>::ConvertToInt24(
						static_cast<int24_t*>(data),
						(int32_t*)buffer,
						context);
					};
				break;
			case 32:
				impl_ = [](void* data, const void* buffer, const AudioConvertContext& context) noexcept {
					DataConverter<PackedFormat::INTERLEAVED, PackedFormat::INTERLEAVED>::ConvertToInt32(
						static_cast<int32_t*>(data),
						(const float*)buffer,
						context);
					};
				break;
			}
		}
		else {
			impl_ = [](void* data, const void* buffer, const AudioConvertContext& context) noexcept {
				DataConverter<PackedFormat::INTERLEAVED, PackedFormat::INTERLEAVED>::ConvertToInt2432(
					static_cast<int32_t*>(data),
					(const float*)buffer,
					context);
				};
		}
	}
	else {
		impl_ = [](void* data, const void* buffer, const AudioConvertContext& context) noexcept {
			DataConverter<PackedFormat::INTERLEAVED, PackedFormat::INTERLEAVED>::Convert(
				static_cast<int16_t*>(data),
				(const float*)buffer,
				context);
			};
	}
}

XAMP_BASE_NAMESPACE_END
