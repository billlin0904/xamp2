#include <algorithm>
#include <player/fft.h>

namespace xamp::player {

FFT::FFT() {
}

void FFT::Process(float *block, size_t size, float scale) {
    std::valarray<Complex> copy(Complex(), size);

    int i = 0;
    std::transform(&copy[0], &copy[size], &copy[0], [&](auto) {
        return Complex(block[i++], 0.0f);
    });

    Forward(copy);

    const float freqoffsets = 2.0f * PI / size;
    const float normfactor  = 2.0f / size;

    for (size_t frame = 0; frame < size; ++frame) {
        block[frame] = 0.5f * copy[0].real();
        for (size_t x = 1; x < copy.size() / 2; ++x) {
            float arg = freqoffsets * x * frame * scale;
            block[frame] += copy[x].real() * std::cos(arg) - copy[x].imag() * std::sin(arg);
        }
        block[frame] *= normfactor;
    }
}

void FFT::Forward(std::valarray<std::complex<float>> &x) {
    const auto N = x.size();
    if (N <= 1) {
        return;
    }

    std::valarray<Complex> even = x[std::slice(0, N / 2, 2)];
    std::valarray<Complex> odd = x[std::slice(1, N / 2, 2)];

    Forward(even);
    Forward(odd);

    for (size_t k = 0; k < N / 2; ++k) {
        auto t = std::polar(1.0f, -2.0f * PI * k / N) * odd[k];
        x[k] = even[k] + t;
        x[k + N / 2] = even[k] - t;
    }
}

void FFT::Inverse(std::valarray<Complex> &x) {
	x = x.apply(std::conj);

	Forward(x);

	x = x.apply(std::conj);
	x /= x.size();
}

}
