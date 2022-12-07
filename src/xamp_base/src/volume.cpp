#include <cstdint>
#include <cmath>
#include <algorithm>

#include <base/volume.h>

namespace xamp::base {

static constexpr float kDbPerStep = 0.5f;
static constexpr float kDbConvert = -kDbPerStep * 2.302585093f / 20.0f;
static constexpr float kDbConvertInverse = 1.0f / kDbConvert;

float VolumeToDb(int32_t volume_level) noexcept {
	volume_level = std::clamp(volume_level, 0, 100);
	return -kDbPerStep * static_cast<float>(100 - volume_level);
}

float LinearToLog(int32_t volume_level) noexcept {	
	return volume_level ? std::exp(static_cast<float>(100 - volume_level) * kDbConvert) : 0;
}

int32_t LogToLineaer(float volume_db) noexcept {
	return volume_db ? 100 - static_cast<int32_t>(kDbConvertInverse * std::log(volume_db) + 0.5) : 0;
}

}
