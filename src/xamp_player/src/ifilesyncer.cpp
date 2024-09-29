#include <fcntl.h>
#include <vector>
#include <fstream>
#include <iostream>

#include <player/ifilesyncer.h>

#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/lockdown.h>
#include <libimobiledevice/afc.h>

#include <base/windows_handle.h>
#include <base/scopeguard.h>
#include <base/unique_handle.h>
#include <base/str_utilts.h>
#include <base/shared_singleton.h>
#include <base/fs.h>
#include <base/dll.h>

#include <base/logger_impl.h>
#include <base/logger.h>

XAMP_AUDIO_PLAYER_NAMESPACE_BEGIN
	class IMobileDeviceLib final {
public:
	IMobileDeviceLib();

	XAMP_DISABLE_COPY(IMobileDeviceLib)

private:
	SharedLibraryHandle module_;

public:
	XAMP_DECLARE_DLL_NAME(idevice_new);
	XAMP_DECLARE_DLL_NAME(lockdownd_client_new_with_handshake);
	XAMP_DECLARE_DLL_NAME(idevice_free);
	XAMP_DECLARE_DLL_NAME(lockdownd_client_free);
	XAMP_DECLARE_DLL_NAME(afc_client_free);
	XAMP_DECLARE_DLL_NAME(lockdownd_service_descriptor_free);
	XAMP_DECLARE_DLL_NAME(lockdownd_start_service);
	XAMP_DECLARE_DLL_NAME(afc_client_new);
	XAMP_DECLARE_DLL_NAME(afc_file_close);
	XAMP_DECLARE_DLL_NAME(afc_file_open);
	XAMP_DECLARE_DLL_NAME(afc_file_write);
	XAMP_DECLARE_DLL_NAME(idevice_set_debug_level);
	XAMP_DECLARE_DLL_NAME(afc_make_directory);
	XAMP_DECLARE_DLL_NAME(idevice_connect);
};

IMobileDeviceLib::IMobileDeviceLib() try
	: module_(LoadSharedLibrary("imobiledevice-1.0.dll"))
	, XAMP_LOAD_DLL_API(idevice_new)
	, XAMP_LOAD_DLL_API(lockdownd_client_new_with_handshake)
	, XAMP_LOAD_DLL_API(idevice_free)
	, XAMP_LOAD_DLL_API(lockdownd_client_free)
	, XAMP_LOAD_DLL_API(afc_client_free)
	, XAMP_LOAD_DLL_API(lockdownd_service_descriptor_free)
	, XAMP_LOAD_DLL_API(lockdownd_start_service)
	, XAMP_LOAD_DLL_API(afc_client_new)
	, XAMP_LOAD_DLL_API(afc_file_close)
	, XAMP_LOAD_DLL_API(afc_file_open)
	, XAMP_LOAD_DLL_API(afc_file_write)
	, XAMP_LOAD_DLL_API(idevice_set_debug_level)
	, XAMP_LOAD_DLL_API(afc_make_directory)
	, XAMP_LOAD_DLL_API(idevice_connect) {
}
catch (const Exception& e) {
	XAMP_LOG_ERROR("{}", e.GetErrorMessage());
}


#define LIB_IMOBILE_DEVICE SharedSingleton<IMobileDeviceLib>::GetInstance()

struct IDeviceDeleter final {
	static idevice_t invalid() noexcept {
		return nullptr;
	}

	static void close(idevice_t value) {
		if (value != nullptr) {
			LIB_IMOBILE_DEVICE.idevice_free(value);
		}
	}
};

using IDeviceHandle = UniqueHandle<idevice_t, IDeviceDeleter>;

struct ILockdownDeleter final {
	static lockdownd_client_t invalid() noexcept {
		return nullptr;
	}

	static void close(lockdownd_client_t value) {
		if (value != nullptr) {
			LIB_IMOBILE_DEVICE.lockdownd_client_free(value);
		}
	}
};

using ILockdownHandle = UniqueHandle<lockdownd_client_t, ILockdownDeleter>;

struct IAfcClientDeleter final {
	static afc_client_t invalid() noexcept {
		return nullptr;
	}

	static void close(afc_client_t value) {
		if (value != nullptr) {
			LIB_IMOBILE_DEVICE.afc_client_free(value);
		}
	}
};
using AfcClientHandle = UniqueHandle<afc_client_t, IAfcClientDeleter>;

struct ILockdowndServiceDescriptorDeleter final {
	static lockdownd_service_descriptor_t invalid() noexcept {
		return nullptr;
	}

	static void close(lockdownd_service_descriptor_t value) {
		if (value != nullptr) {
			LIB_IMOBILE_DEVICE.lockdownd_service_descriptor_free(value);
		}
	}
};

using ILockdowndServiceDescriptor = UniqueHandle<lockdownd_service_descriptor_t, ILockdowndServiceDescriptorDeleter>;

const auto kIPhoneMusicPath = Path(L"/private/var/mobile/Media/iTunes Control/Music/");

class ConsoleAdapter {
public:
	ConsoleAdapter() {
	}
	~ConsoleAdapter() {
	}
private:
};

class ITunesFileSyncer::ITunesFileSyncerImpl {
public:
	ITunesFileSyncerImpl() {
		LIB_IMOBILE_DEVICE.idevice_set_debug_level(1);

		idevice_t device = nullptr;
		auto error = LIB_IMOBILE_DEVICE.idevice_new(&device, nullptr);
		if (error != IDEVICE_E_SUCCESS) {
			throw std::runtime_error("Failed to create device handle");
		}
		device_.reset(device);

		lockdownd_client_t lockdown = nullptr;
		auto lockdown_error = LIB_IMOBILE_DEVICE.lockdownd_client_new_with_handshake(device, 
			&lockdown,
			"iTunesFileSyncer");
		if (lockdown_error != LOCKDOWN_E_SUCCESS) {
			throw std::runtime_error("Failed to create lockdown client");
		}
		lockdown_.reset(lockdown);
	}

	void AddMusicToLibrary(const std::wstring& file_path) {
		lockdownd_service_descriptor_t service = nullptr;

		auto error = LIB_IMOBILE_DEVICE.lockdownd_start_service(lockdown_.get(), "com.apple.afc", &service);
		if (error != LOCKDOWN_E_SUCCESS) {
			throw std::runtime_error("Failed to start AFC service");
		}
		ILockdowndServiceDescriptor service_descriptor(service);

		idevice_connection_t connection = nullptr;
		auto device_error= LIB_IMOBILE_DEVICE.idevice_connect(device_.get(), service_descriptor.get()->port, &connection);
		if (device_error != IDEVICE_E_SUCCESS) {
			throw std::runtime_error("Failed to connect device");
		}

		std::this_thread::sleep_for(std::chrono::seconds(3));

		afc_client_t afc = nullptr;
		auto afc_error = LIB_IMOBILE_DEVICE.afc_client_new(device_.get(), service, &afc);
		if (afc_error != AFC_E_SUCCESS) {
			throw std::runtime_error("Failed to create AFC client");
		}
		AfcClientHandle afc_client_handle(afc);

		/*afc_error = LIB_IMOBILE_DEVICE.afc_make_directory(afc, kIPhoneMusicPath.string().c_str());
		if (afc_error != AFC_E_SUCCESS) {
			throw std::runtime_error("Failed to create directory");
		}*/

		const auto remote_file_path = kIPhoneMusicPath / Path(file_path).filename().wstring();
		const auto file_path_utf8 = String::ToUtf8String(remote_file_path);
		uint64_t afc_file = 0;
		afc_error = LIB_IMOBILE_DEVICE.afc_file_open(afc, file_path_utf8.c_str(), AFC_FOPEN_WRONLY, &afc_file);
		if (afc_error != AFC_E_SUCCESS) {
			throw std::runtime_error("Failed to open file");
		}

		XAMP_ON_SCOPE_EXIT([&] {
			LIB_IMOBILE_DEVICE.afc_file_close(afc, afc_file);
			afc_client_handle.reset();
			service_descriptor.reset();
			});

		std::vector<char> buffer(4096);

		std::ifstream local_file(Path(file_path), std::ios::binary);
		while (local_file.read(buffer.data(), buffer.size()) || local_file.gcount() > 0) {
			uint32_t bytes_written;
			if (LIB_IMOBILE_DEVICE.afc_file_write(afc, afc_file, buffer.data(), static_cast<uint32_t>(local_file.gcount()), &bytes_written) != AFC_E_SUCCESS) {
				return;
			}
		}
	}

	void SyncToDevice() {
		
	}
private:
	ConsoleAdapter console_;
	IDeviceHandle device_;
	ILockdownHandle lockdown_;
};

ITunesFileSyncer::ITunesFileSyncer()
	: impl_(MakeAlign<ITunesFileSyncerImpl>()) {
}

XAMP_PIMPL_IMPL(ITunesFileSyncer)

void ITunesFileSyncer::AddMusicToLibrary(const std::wstring& file_path) {
	impl_->AddMusicToLibrary(file_path);
}

void ITunesFileSyncer::SyncToDevice() {
	impl_->SyncToDevice();
}

XAMP_AUDIO_PLAYER_NAMESPACE_END