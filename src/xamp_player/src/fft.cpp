#include <player/fft.h>

namespace xamp::player {

void FFT::Forward(std::valarray<std::complex<float>> &x) {
	const auto N = x.size();
	if (N <= 1) {
		return;
	}

	std::valarray<std::complex<float>> even = x[std::slice(0, N / 2, 2)];
	std::valarray<std::complex<float>> odd = x[std::slice(1, N / 2, 2)];

	Forward(even);
	Forward(odd);

	for (size_t k = 0; k < N / 2; ++k) {
		auto t = std::polar(1.0f, -2.0f * PI * k / N) * odd[k];
		x[k] = even[k] + t;
		x[k + N / 2] = even[k] - t;
	}
}

void FFT::Inverse(std::valarray<std::complex<float>> &x) {
	x = x.apply(std::conj);
	Forward(x);
	x = x.apply(std::conj);
	x /= x.size();
}

}