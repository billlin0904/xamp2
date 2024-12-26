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
 * 將 0~100 之間的 volume level 轉成 dB
 * 其中 level=100 -> 0 dB, level=0 -> -50 dB
 * 最後做四捨五入(整數 dB)
 */
float VolumeLevelToDb(int32_t volume_level) {
	volume_level = std::clamp(volume_level, 0, 100);

	// 原公式: dB = -0.5f * (100 - volume_level)
	float db = -kDbPerStep * static_cast<float>(100 - volume_level);

	// 四捨五入(整數)
	db = std::roundf(db);

	return db;
}

/**
 * 將 0~100 之間的 volume level 轉成增益(倍數)
 *   其中 level=100 => gain=1.0, level=0 => gain=10^(-50/20) = 0.00316...
 * 最後保留小數第3位為例
 */
float VolumeLevelToGain(int32_t volume_level) {	
	volume_level = std::clamp(volume_level, 0, 100);

	// exponent = (100 - level) * kDbConvert
	// gain = e^(exponent)
	float exponent = (100 - volume_level) * kDbConvert;
	float gain = std::exp(exponent);

	// 例如:四捨五入到小數3位 => 乘1000後 round 再除回
	gain = std::roundf(gain * 1000.0f) / 1000.0f;

	return gain;
}

int32_t GainToVolumeLevel(float volume_db) {
	return volume_db ? 100 - static_cast<int32_t>(kDbConvertInverse * std::log(volume_db) + 0.5) : 0;
}

XAMP_BASE_NAMESPACE_END
