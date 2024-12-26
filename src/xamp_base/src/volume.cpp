#include <base/volume.h>

#include <cstdint>
#include <cmath>
#include <algorithm>
#include <numbers>

XAMP_BASE_NAMESPACE_BEGIN

namespace {
	constexpr float LN10 = std::numbers::ln10_v<float>;

	constexpr float kDbPerStep = 0.5f;
	constexpr float kDbConvert = -kDbPerStep * LN10 / 20.0f;
	constexpr float kDbConvertInverse = 1.0f / kDbConvert;
}

/**
 * �N 0~100 ������ volume level �ন dB
 * �䤤 level=100 -> 0 dB, level=0 -> -50 dB
 * �̫ᰵ�|�ˤ��J(��� dB)
 */
float VolumeLevelToDb(int32_t volume_level) {
	volume_level = std::clamp(volume_level, 0, 100);

	// �줽��: dB = -0.5f * (100 - volume_level)
	float db = -kDbPerStep * static_cast<float>(100 - volume_level);

	// �|�ˤ��J(���)
	db = std::roundf(db);

	return db;
}

/**
 * �N 0~100 ������ volume level �ন�W�q(����)
 *   �䤤 level=100 => gain=1.0, level=0 => gain=10^(-50/20) = 0.00316...
 * �̫�O�d�p�Ʋ�3�쬰��
 */
float VolumeLevelToGain(int32_t volume_level) {	
	volume_level = std::clamp(volume_level, 0, 100);

	// exponent = (100 - level) * kDbConvert
	// gain = e^(exponent)
	float exponent = (100 - volume_level) * kDbConvert;
	float gain = std::exp(exponent);

	// �Ҧp:�|�ˤ��J��p��3�� => ��1000�� round �A���^
	gain = std::roundf(gain * 1000.0f) / 1000.0f;

	return gain;
}

int32_t GainToVolumeLevel(float volume_db) {
	return volume_db ? 100 - static_cast<int32_t>(kDbConvertInverse * std::log(volume_db) + 0.5) : 0;
}

XAMP_BASE_NAMESPACE_END
