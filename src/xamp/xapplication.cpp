#include <QFileInfo>
#include <QLocalServer>
#include <QLocalSocket>
#include <QSysInfo>

#include <base/logger_impl.h>
#include <base/platform.h>
#include <base/crashhandler.h>
#include <base/platform.h>
#include <base/dll.h>

#include <widget/database.h>
#include <widget/xmainwindow.h>
#include <widget/appsettings.h>
#include <widget/jsonsettings.h>

#include <FramelessHelper/Widgets/framelessmainwindow.h>
#include <FramelessHelper/Core/private/framelessconfig_p.h>

#include <xapplication.h>
#include <widget/youtubedl/ytmusic_disckcache.h>

constexpr auto kIpcTimeout = 1000;

XAMP_DECLARE_LOG_NAME(XApplication);

namespace {
	Vector<SharedLibraryHandle> prefetchDll() {
		// 某些DLL無法在ProcessMitigation 再次載入但是這些DLL都是必須要的.               
		const Vector<std::string_view> dll_file_names{
			R"(WS2_32.dll)",
			R"(Python3.dll)",
			R"(mimalloc-override.dll)",
			R"(C:\Program Files\Topping\USB Audio Device Driver\x64\ToppingUsbAudioasio_x64.dll)",
			R"(C:\Program Files\iFi\USB_HD_Audio_Driver\iFiHDUSBAudioasio_x64.dll)",
			R"(C:\Program Files\FiiO\FiiO_Driver\W10_x64\fiio_usbaudioasio_x64.dll)",
			R"(C:\Program Files\Bonjour\mdnsNSP.dll)",
		};
		Vector<SharedLibraryHandle> preload_module;
#ifdef Q_OS_WIN
		for (const auto& file_name : dll_file_names) {
			try {
				auto module = LoadSharedLibrary(file_name);
				if (PrefetchSharedLibrary(module)) {
					preload_module.push_back(std::move(module));
					XAMP_LOG_DEBUG("\tPreload => {} success.", file_name);
				}
			}
			catch (const Exception& e) {
				XAMP_LOG_DEBUG("Preload {} failure! {}.", file_name, e.GetErrorMessage());
			}
		}		
#endif
		return preload_module;
	}
}

XApplication::XApplication(int& argc, char* argv[])
	: QApplication(argc, argv) {
	logger_ = XampLoggerFactory.GetLogger(XAMP_LOG_NAME(XApplication));

	QLocalSocket socket;
	socket.connectToServer(applicationName());

	if (socket.waitForConnected(kIpcTimeout)) {
		is_running_ = true;
		return;																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																											
	}

	server_.reset(new QLocalServer(this));
	(void)QObject::connect(server_.get(), &QLocalServer::newConnection, [this]() {
		QScopedPointer<QLocalSocket> socket(server_->nextPendingConnection());
		if (socket) {
			socket->waitForReadyRead(2 * kIpcTimeout);
			socket.reset();
			if (!window_) {
				return;
			}
			window_->showWindow();
		}
	});

	if (!server_->listen(applicationName())) {
		if (server_->serverError() == QAbstractSocket::AddressInUseError) {
			QLocalServer::removeServer(applicationName());
			server_->listen(applicationName());
		}
	}

	FramelessHelper::Widgets::initialize();
	FramelessHelper::Core::setApplicationOSThemeAware();
	FramelessConfig::instance()->set(Global::Option::DisableWindowsSnapLayout);
	FramelessConfig::instance()->set(Global::Option::EnableBlurBehindWindow);

	//SetCurrentProcessPriority(ProcessPriority::PRIORITY_BACKGROUND);
}

XApplication::~XApplication() {
	FramelessHelper::Widgets::uninitialize();
}

bool XApplication::isAttach() const {
    return !is_running_;
}

void XApplication::initial() {
	qAppSettings.loadIniFile(qTEXT("xamp.ini"));
	qJsonSettings.loadJsonFile(qTEXT("config.json"));

	qAppSettings.loadOrSaveLogConfig();
	qAppSettings.loadAppSettings();

	qDiskCache.load();

	XampCrashHandler.SetProcessExceptionHandlers();
	XAMP_LOG_DEBUG("SetProcessExceptionHandlers success.");

	XampCrashHandler.SetThreadExceptionHandlers();
	XAMP_LOG_DEBUG("SetThreadExceptionHandlers success.");

#ifdef Q_OS_WIN32
	const auto components_path = GetComponentsFilePath();
	if (!AddSharedLibrarySearchDirectory(components_path)) {
		throw Exception(String::Format("AddSharedLibrarySearchDirectory return fail! ({})", GetLastErrorMessage()));		
	}

	auto prefetch_dll = prefetchDll();
	XAMP_LOG_DEBUG("Prefetch dll success.");
#endif    
}

void XApplication::loadSampleRateConverterConfig() {
	XAMP_LOG_DEBUG("LoadSampleRateConverterConfig.");
	qAppSettings.loadSoxrSetting();
	qAppSettings.LoadR8BrainSetting();
	qJsonSettings.save();
	XAMP_LOG_DEBUG("loadLogAndSoxrConfig success.");
}

void XApplication::applyTheme() {
	const auto theme = qAppSettings.valueAsEnum<ThemeColor>(kAppSettingTheme);
	qTheme.setThemeColor(theme);
	qTheme.loadAndApplyTheme();
}

void XApplication::loadLang() {
	XAMP_LOG_DEBUG("Load language file.");

	if (qAppSettings.valueAsString(kAppSettingLang).isEmpty()) {
		const LocaleLanguage lang;
		XAMP_LOG_DEBUG("Load locale language file: {}.", lang.isoCode().toStdString());
		qAppSettings.loadLanguage(lang.isoCode());
		qAppSettings.loadLanguage(qSTR("qt_%1").arg(lang.isoCode()));
		qAppSettings.setValue(kAppSettingLang, lang.isoCode());
	}
	else {
		qAppSettings.loadLanguage(qAppSettings.valueAsString(kAppSettingLang));
		XAMP_LOG_DEBUG("Load locale language file: {}.",
			qAppSettings.valueAsString(kAppSettingLang).toStdString());
	}
}
