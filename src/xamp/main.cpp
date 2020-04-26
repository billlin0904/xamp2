#include <cstdio>

#include <base/logger.h>
#include <base/rng.h>
#include <base/dll.h>
#include <base/vmmemlock.h>
#include <base/stacktrace.h>

#include <player/audio_player.h>
#include <widget/qdebugsink.h>
#include <widget/appsettings.h>
#include <widget/database.h>
#include <widget/pixmapcache.h>
#include <widget/str_utilts.h>
#include <widget/jsonsettings.h>

#include <QMessageBox>
#include <QtWidgets/QApplication>

#include "thememanager.h"
#include "singleinstanceapplication.h"
#include "xamp.h"

void loadOrDefaultSoxrSetting() {
	if (JsonSettings::keys().count() > 0) {
		return;
	}

	QMap<QString, QVariant> defaultSetting;

	defaultSetting[SOXR_RESAMPLE_SAMPLRATE] = 44100;
	defaultSetting[SOXR_ENABLE_STEEP_FILTER] = false;
	defaultSetting[SOXR_QUALITY] = 3;
	defaultSetting[SOXR_PHASE] = 0;
	defaultSetting[SOXR_PASS_BAND] = 91;

	JsonSettings::setValue(SOXR_DEFAULT_SETTING_NAME, QVariant::fromValue(defaultSetting));
	AppSettings::setValue(APP_SETTING_SOXR_SETTING_NAME, SOXR_DEFAULT_SETTING_NAME);
	AppSettings::setDefaultValue(APP_SETTING_SOXR_SETTING_NAME, SOXR_DEFAULT_SETTING_NAME);

	JsonSettings::save();
}

static int excute(int argc, char* argv[]) {
	Logger::Instance()
#ifdef Q_OS_WIN
		.AddDebugOutputLogger()
#else
		.AddSink(std::make_shared<QDebugSink>())
#endif
        .AddFileLogger("xamp.log");

    XAMP_SET_LOG_LEVEL(spdlog::level::debug);

	XAMP_LOG_DEBUG("Logger init success.");

	StackTrace::RegisterAbortHandler();

	XAMP_LOG_DEBUG("RegisterAbortHandler success.");

	std::vector<ModuleHandle> preload_modules;

	const std::vector<std::string_view> preload_modules_names {
		#ifdef Q_OS_WIN
		// 如果沒有預先載入拖入檔案會爆音.
		"psapi.dll",
		"comctl32.dll",
		"WindowsCodecs.dll",
		// 為了效率考量.
		"AUDIOKSE.dll",
		"avrt.dll"
		#else
		"libchromaprint.dylib",
		"libbass.dylib"
		#endif
	};

	QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

	QApplication app(argc, argv);

	try {
		for (auto name : preload_modules_names) {
			preload_modules.emplace_back(LoadModule(name));
		}
		AudioPlayer::LoadLib();
#ifdef Q_OS_WIN
		VmMemLock::EnableLockMemPrivilege(true);
#endif
	}
	catch (const Exception& e) {
		QMessageBox::critical(nullptr, Q_UTF8("Load dll failure!"), QString::fromStdString(e.GetErrorMessage()));
		return -1;
	}

	XAMP_LOG_DEBUG("Preload dll success.");	

	SingleInstanceApplication singleApp;
	if (!singleApp.attach(QCoreApplication::arguments())) {
		return -1;
	}

	XAMP_LOG_DEBUG("attach app success.");

	(void)RNG::Instance();
	(void)PixmapCache::instance();

    XAMP_LOG_DEBUG("PixmapCache init success.");

	try {
		Database::instance().open(Q_UTF8("xamp.db"));
	}
	catch (const std::exception& e) {
		XAMP_LOG_INFO("Initial database failure! {}", e.what());
		return -1;
	}

	XAMP_LOG_DEBUG("Database init success.");
	
	AppSettings::setOrDefaultConfig();
	JsonSettings::loadJsonFile(Q_UTF8("soxr.json"));
	loadOrDefaultSoxrSetting();

	XAMP_LOG_DEBUG("setOrDefaultConfig success.");

	Xamp win;
	win.initial();
	win.show();
	return app.exec();
}

#ifdef XAMP_OS_WIN
static int tryExcute(int argc, char* argv[]) {
	DWORD code = 0;
	LPEXCEPTION_POINTERS info = nullptr;

	__try {
		return excute(argc, argv);
	}
	__except (code = GetExceptionCode(), info = GetExceptionInformation(), EXCEPTION_EXECUTE_HANDLER) {
		char buffer[256];
		sprintf_s(buffer, sizeof(buffer), "Exception code: 0x%08x", code);
		::MessageBoxA(nullptr, buffer, "Something wrong!", MB_OK);
		return -1;
	}
}
#else
static int tryExcute(int argc, char* argv[]) {
    return excute(argc, argv);
}
#endif

int main(int argc, char *argv[]) {
    auto result = tryExcute(argc, argv);
    Logger::Instance().Shutdown();
    return result;
}
