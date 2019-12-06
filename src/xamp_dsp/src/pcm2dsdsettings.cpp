#include <dsp/coefs.h>
#include <dsp/dsd2pcmsettings.h>

namespace xamp::dsp {

static CoefsTable CreateTable(const int32_t* fir_coefs, int32_t fir_length, float fir_gain) {
	auto ctables = GetCoefsTableSize(fir_length);

	CoefsTable table;
	table.resize(ctables);

	for (auto ct = 0; ct < ctables; ct++) {
		auto k = fir_length - ct * 8;
		if (k > 8) {
			k = 8;
		}

		if (k < 0) {
			k = 0;
		}

		for (auto i = 0; i < table[ct].size(); i++) {
			double cvalue = 0.0;
			for (int j = 0; j < k; j++) {
				cvalue += (((i >> (7 - j)) & 1) * 2 - 1)* fir_coefs[fir_length - 1 - (ct * 8 + j)];
			}
			table[ct][i] = (float)(cvalue * fir_gain);
		}
	}

	return table;
}

Dsd2PcmSettings::Dsd2PcmSettings() {
}

CoefsFirTable Dsd2PcmSettings::GetDsdFir1_64() const {
	return CoefsFirTable {
		DSDFIR1_64_LENGTH,
		CreateTable(DSDFIR1_64_COEFS, DSDFIR1_64_LENGTH, 1.0)
	};
}

}