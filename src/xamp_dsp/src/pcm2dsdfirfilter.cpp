#include <dsp/coefs.h>
#include <dsp/dsd2pcmfirfilter.h>

namespace xamp::dsp {

Dsd2PcmFirFilter::Dsd2PcmFirFilter()
	: decimation_(0)
	, fir_index_(0)
	, fir_length_(0) {
}

Dsd2PcmFirFilter::~Dsd2PcmFirFilter() {
}

int32_t Dsd2PcmFirFilter::GetDelay() const {
	return (fir_length_ - 1) / 2 / 8 / decimation_;
}

void Dsd2PcmFirFilter::Initial(const CoefsFirTable& fir_table, int32_t decimation) {
	fir_table_ = fir_table;
	decimation_ = decimation / 8;
	fir_length_ = GetCoefsTableSize(fir_table.fir_length);
	size_t buf_size = 2 * fir_length_ * sizeof(uint8_t);
	fir_buffer_ = MakeBuffer<uint8_t>(buf_size);
	fir_index_ = 0;
}

int32_t Dsd2PcmFirFilter::operator()(float* pcm_samples, const uint8_t* dsd_samples, int32_t samples) {
	auto pcm_samples_size = samples / decimation_;

	for (auto sample = 0; sample < pcm_samples_size; sample++) {
		for (auto i = 0; i < decimation_; i++) {
			fir_buffer_[fir_index_ + fir_length_] = fir_buffer_[fir_index_] = *(dsd_samples++);
			fir_index_ = (++fir_index_) % fir_length_;
		}
		pcm_samples[sample] = 0;
		for (auto j = 0; j < fir_length_; j++) {
			pcm_samples[sample] += fir_table_.ctable[j][fir_buffer_[fir_index_ + j]];
		}
	}

	return pcm_samples_size;
}

}
