#include <base/dataconverter.h>

namespace xamp::base {

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

#define ClampSampleRAW(f) std::clamp(f, kMinFloatSample, kMaxFloatSample)

#ifdef XAMP_OS_WIN
float FloatMinSSE2(float a, float b) noexcept {
	::_mm_store_ss(&a, ::_mm_min_ss(::_mm_set_ss(a), ::_mm_set_ss(b)));
	return a;
}

float FloatMaxSSE2(float a, float b) noexcept {
	::_mm_store_ss(&a, ::_mm_max_ss(::_mm_set_ss(a), ::_mm_set_ss(b)));
	return a;
}

inline float ClampSampleSSE2(float val, __m128 minval, __m128 maxval) noexcept {
	::_mm_store_ss(&val, ::_mm_min_ss(::_mm_max_ss(::_mm_set_ss(val), minval), maxval));
	return val;
}
	
void ClampSample(float* f, size_t num_samples) noexcept {
	const auto min_value = ::_mm_set_ss(kMinFloatSample);
	const auto max_value = ::_mm_set_ss(kMaxFloatSample);

	const auto* end_input = f + num_samples;
	switch ((end_input - f) % kLoopUnRollingIntCount) {
	case 3:
		*f = ClampSampleSSE2(*f, min_value, max_value);
		++f;
		[[fallthrough]];
	case 2:
		*f = ClampSampleSSE2(*f, min_value, max_value);
		++f;
		[[fallthrough]];
	case 1:
		*f = ClampSampleSSE2(*f, min_value, max_value);
		++f;
		[[fallthrough]];
	case 0:
		break;
	}

	while (f != end_input) {
		::_mm_prefetch(reinterpret_cast<char const*>(f), _MM_HINT_T0);
		auto r0 = f[0];
		auto r1 = f[1];
		auto r2 = f[2];
		auto r3 = f[3];
		f[0] = ClampSampleSSE2(r0, min_value, max_value);
		f[1] = ClampSampleSSE2(r1, min_value, max_value);
		f[2] = ClampSampleSSE2(r2, min_value, max_value);
		f[3] = ClampSampleSSE2(r3, min_value, max_value);
		f += 4;
	}
}

float ClampSampleSSE2(float f) noexcept {
	const auto min_value = ::_mm_set_ss(kMinFloatSample);
	const auto max_value = ::_mm_set_ss(kMaxFloatSample);
	return ClampSampleSSE2(f, min_value, max_value);
}
#else
void ClampSample(float* f, size_t num_samples) noexcept {
	const auto* end_input = f + num_samples;

	switch ((end_input - f) % kLoopUnRollingIntCount) {
	case 3:
		*f = ClampSampleRAW(*f);
		++f;
		[[fallthrough]];
	case 2:
		*f = ClampSampleRAW(*f);
		++f;
		[[fallthrough]];
	case 1:
		*f = ClampSampleRAW(*f);
		++f;
		[[fallthrough]];
	case 0:
		break;
	}

	while (f != end_input) {
		f[0] = ClampSampleRAW(f[0]);
		f[1] = ClampSampleRAW(f[1]);
		f[2] = ClampSampleRAW(f[2]);
		f[3] = ClampSampleRAW(f[3]);
		f += 4;
	}
}

float ClampSampleSSE2(float f) noexcept {
	return ClampSampleRAW(f);
}

float FloatMax(float a, float b) noexcept {
	return std::max(a, b);
}
#endif

}

