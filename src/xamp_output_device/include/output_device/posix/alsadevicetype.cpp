#include <base/unique_handle.h>
#include <output_device/posix/alsadevicetype.h>

#ifdef XAMP_OS_LINUX
namespace xamp::output_device::posix {

struct snd_ctl_t {};
struct snd_pcm_t {};
struct snd_pcm_hw_params_t{};
struct snd_pcm_stream_t {};
struct snd_pcm_info_t {};
	
void snd_ctl_close(snd_ctl_t*);
void snd_pcm_close(snd_pcm_t*);
int snd_ctl_open(snd_ctl_t**, char const *, int);
void snd_pcm_info_alloca(snd_pcm_info_t**);
void snd_pcm_hw_params_alloca(snd_pcm_hw_params_t**);
void snd_pcm_info_set_stream(snd_pcm_info_t*, snd_pcm_stream_t);
void snd_pcm_info_set_device(snd_pcm_info_t*, int);
void snd_pcm_info_set_subdevice(snd_pcm_info_t*, int);
	
void snd_card_next(int*);
int snd_ctl_pcm_next_device(snd_ctl_t*, int*);
#define SND_CTL_NONBLOCK 1
#define SND_PCM_ASYNC 1

struct SndPcmHandleDeleter final {
	static snd_pcm_t* invalid() noexcept {
		return nullptr;
	}

	static void close(snd_pcm_t* value) {
		snd_pcm_close(value);
	}
};
	
struct SndCtrlHandleDeleter final {
	static snd_ctl_t* invalid() noexcept {
		return nullptr;
	}
	
	static void close(snd_ctl_t* value) {
		snd_ctl_close(value);
	}
};

using SndCtrlHandle = UniqueHandle<snd_ctl_t*, SndCtrlHandleDeleter>;
using SndPcmHandle = UniqueHandle<snd_pcm_t*, SndPcmHandleDeleter>;

AlsaDeviceType::~AlsaDeviceType() {
}

void AlsaDeviceType::ScanNewDevice() {
	
}

std::string_view AlsaDeviceType::GetDescription() const {
	return "ALSA";
}

Uuid AlsaDeviceType::GetTypeId() const {
	return Id;
}

AlignPtr<Device> AlsaDeviceType::MakeDevice(std::string const& device_id) {
	
}

size_t AlsaDeviceType::GetDeviceCount() const {
	char name[64] = "default";
	size_t device_count = 0;
	
	snd_ctl_t* result = nullptr;	
	auto rs = snd_ctl_open(&result, "default", 0);	
	if (rs == 0) {
		SndCtrlHandle handle(result);
		++device_count;
	}
		
	int card = -1;
	snd_card_next(&card);
	while (card >= 0) {
		snd_ctl_t* result = nullptr;
		auto res = snd_ctl_open(&result, name, 0);
		if (res < 0) {
			break;
		}
		SndCtrlHandle handle(result);
		auto subdevice = -1;
		while (true) {
			res = snd_ctl_pcm_next_device(result, &subdevice);
			if (res < 0 || subdevice < 0) {
				break;
			}
			++device_count;
		}
	}
	return device_count;
}

std::vector<DeviceInfo> AlsaDeviceType::GetDeviceInfo() const {
	char name[64] = "default";

	snd_ctl_t* result = nullptr;
	auto rs = snd_ctl_open(&result, "default", SND_CTL_NONBLOCK);
	if (rs == 0) {
		SndCtrlHandle handle(result);
	}

	auto subdevice = -1;
	auto card = -1;
	snd_card_next(&card);
	while (card >= 0) {
		snd_ctl_t* result = nullptr;
		auto res = snd_ctl_open(&result, name, SND_CTL_NONBLOCK);
		if (res < 0) {
			break;
		}
		SndCtrlHandle handle(result);		
		while (true) {
			res = snd_ctl_pcm_next_device(result, &subdevice);
			if (res < 0 || subdevice < 0) {
				break;
			}
		}
	}

	int openMode = SND_PCM_ASYNC;
	snd_pcm_stream_t stream;
	snd_pcm_info_t* pcminfo;
	snd_pcm_info_alloca(&pcminfo);
	snd_pcm_t* phandle;
	snd_pcm_hw_params_t* params;
	snd_pcm_hw_params_alloca(&params);

	snd_pcm_info_set_stream(pcminfo, stream);
	if (subdevice != -1) {
		snd_pcm_info_set_device(pcminfo, subdevice);
		snd_pcm_info_set_subdevice(pcminfo, 0);		
	}
}

DeviceInfo AlsaDeviceType::GetDeviceInfo(uint32_t device) const {
	
}

}
#endif