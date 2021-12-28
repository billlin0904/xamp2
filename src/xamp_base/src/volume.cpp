#include <base/volume.h>

namespace xamp::base {

static constexpr float kDbPerStep = 0.5f;
static constexpr float kDbConvert = -kDbPerStep * 2.302585093f / 20.0f;
static constexpr float kDbConvertInverse = 1.0f / kDbConvert;

float VolumeToDb(int32_t volume) noexcept {
	volume = std::clamp(volume, 0, 100);	
	return -kDbPerStep * static_cast<float>(100 - volume);
}

float LinearToLog(int32_t volume) noexcept {	
	return volume ? std::exp(static_cast<float>(100 - volume) * kDbConvert) : 0;
}

int32_t LogToLineaer(float volume) noexcept {
	return volume ? 100 - static_cast<int32_t>(kDbConvertInverse * std::log(volume) + 0.5) : 0;
}

}
