#pragma once

#include <output_device/iaudiocallback.h>
#include <output_device/win32/mmcss.h>
#include <output_device/win32/comexception.h>
#include <output_device/win32/wasapi.h>

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_BEGIN

struct AudioGlitchInfo {
	using Duration = std::chrono::milliseconds;

	Duration duration{}; // 累積的 glitch 時間
	uint32_t count{ 0 };   // glitch 次數

	AudioGlitchInfo() = default;

	// 建立一個「單一系統層級的 glitch」資訊，duration 會被 clamp 在 [0, 1s]
	static AudioGlitchInfo SingleBoundedSystemGlitch(Duration duration);

	AudioGlitchInfo& operator+=(const AudioGlitchInfo& other) {
		duration += other.duration;
		count += other.count;
		return *this;
	}
};

struct Accumulator {
	Accumulator() = default;
	~Accumulator() = default;

	void Add(const AudioGlitchInfo& info) {
		pending_info_ += info;
	}

	AudioGlitchInfo GetAndReset() {
		AudioGlitchInfo tmp = pending_info_;
		pending_info_ = {};
		return tmp;
	}

private:
	AudioGlitchInfo pending_info_;
};

inline bool operator==(const AudioGlitchInfo& lhs, const AudioGlitchInfo& rhs) {
	return lhs.duration == rhs.duration && lhs.count == rhs.count;
}

// static
inline AudioGlitchInfo AudioGlitchInfo::SingleBoundedSystemGlitch(Duration duration) {
	using namespace std::chrono;
	// 負值當成 0，超過 1 秒就 clamp
	const auto clamped =
		std::clamp(duration, Duration::zero(), Duration(1000));

	AudioGlitchInfo info;
	info.duration = clamped;
	info.count = 1;
	return info;
}

class GlitchDetector {
public:
	using Duration = std::chrono::milliseconds;

	GlitchDetector() = default;

	void Reset(uint64_t device_frequency, Duration buffer_duration) ;

	std::optional<AudioGlitchInfo> Update(uint64_t position, uint64_t qpc_position_100ns) ;

	AudioGlitchInfo GetTotalGlitchInfoAndReset() ;

private:
	uint64_t device_frequency_{ 0 }; // IAudioClock::GetFrequency()
	Duration buffer_duration_{ 0 };

	uint64_t last_position_{ 0 };
	uint64_t last_qpc_position_{ 0 };

	Accumulator total_glitch_accumulator_;
};

inline void GlitchDetector::Reset(uint64_t device_frequency, Duration buffer_duration) {
	device_frequency_ = device_frequency;
	buffer_duration_ = buffer_duration;
	last_position_ = 0;
	last_qpc_position_ = 0;
	total_glitch_accumulator_ = Accumulator{};
}

inline std::optional<AudioGlitchInfo> GlitchDetector::Update(uint64_t position, uint64_t qpc_position_100ns) {
	// 尚未初始化或參數異常就直接跳過
	if (device_frequency_ == 0) {
		return std::nullopt;
	}

	// 第一次呼叫，只記錄基準
	if (last_position_ == 0) {
		last_position_ = position;
		last_qpc_position_ = qpc_position_100ns;
		return std::nullopt;
	}

	// position 理論上要單調不減，如果倒退就當成 0（Chromium 也是這樣處理部分異常）
	int64_t position_delta = static_cast<int64_t>(position) -
		static_cast<int64_t>(last_position_);
	if (position_delta < 0) {
		position_delta = 0;
	}

	// 將 position 的差值轉成時間（device_frequency 單位同 position）
	// duration_in_seconds = delta / freq
	std::chrono::duration<double> position_time_increase =
		std::chrono::duration<double>(static_cast<double>(position_delta) /
			static_cast<double>(device_frequency_));

	// QPC 差值（100ns tick -> microseconds）
	int64_t qpc_delta_100ns = static_cast<int64_t>(qpc_position_100ns) -
		static_cast<int64_t>(last_qpc_position_);
	if (qpc_delta_100ns < 0) {
		// 如果 QPC 倒退，就當成 0
		qpc_delta_100ns = 0;
	}

	std::chrono::microseconds qpc_time_increase(qpc_delta_100ns / 10); // 10 * 100ns = 1us

	// gap = "實際時間" - "device position 對應的時間"
	// gap 很大 => position 前進不足 => 有一段時間沒有播放 => 可能 glitch
	auto gap_duration =
		duration_cast<std::chrono::milliseconds>(qpc_time_increase - position_time_increase);

	// 閾值：半個 buffer duration（跟 Chromium 相同 heuristics）
	const auto glitch_threshold = buffer_duration_ / 2;

	std::optional<AudioGlitchInfo> result;

	if (gap_duration > glitch_threshold) {
		// 記錄為 glitch，clamp 在 [0, 1s]，count=1
		auto info = AudioGlitchInfo::SingleBoundedSystemGlitch(gap_duration);
		total_glitch_accumulator_.Add(info);
		result = info;
	}

	last_position_ = position;
	last_qpc_position_ = qpc_position_100ns;

	return result;
}

inline AudioGlitchInfo GlitchDetector::GetTotalGlitchInfoAndReset() {
	return total_glitch_accumulator_.GetAndReset();
}

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_END
