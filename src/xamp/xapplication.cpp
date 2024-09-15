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

#include <xapplication.h>
#include <widget/youtubedl/ytmusic_disckcache.h>

constexpr auto kIpcTimeout = 1000;

XAMP_DECLARE_LOG_NAME(XApplication);

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
	//SetCurrentProcessPriority(ProcessPriority::PRIORITY_BACKGROUND);
}

XApplication::~XApplication() = default;

bool XApplication::isAttach() const {
    return !is_running_;
}

void XApplication::initial() {
	qAppSettings.loadIniFile("xamp.ini"_str);
	qJsonSettings.loadJsonFile("config.json"_str);

	qAppSettings.loadOrSaveLogConfig();
	qAppSettings.loadAppSettings();

	qDiskCache.load();   
}

void XApplication::loadSampleRateConverterConfig() {
	XAMP_LOG_DEBUG("LoadSampleRateConverterConfig.");
	qAppSettings.loadSoxrSetting();
	qAppSettings.LoadR8BrainSetting();
	qJsonSettings.save();
	XAMP_LOG_DEBUG("loadLogAndSoxrConfig success.");
}

void XApplication::setTheme() {
	qTheme.loadAndSetThemeQss();
	const auto theme = qAppSettings.valueAsEnum<ThemeColor>(kAppSettingTheme);
	qTheme.setThemeColor(theme);
}

void XApplication::loadLang() {
	XAMP_LOG_DEBUG("Load language file.");

	if (qAppSettings.valueAsString(kAppSettingLang).isEmpty()) {
		const LocaleLanguage lang;
		XAMP_LOG_DEBUG("Load locale language file: {}.", lang.isoCode().toStdString());
		qAppSettings.loadLanguage(lang.isoCode());
		qAppSettings.loadLanguage(qFormat("qt_%1").arg(lang.isoCode()));
		qAppSettings.setValue(kAppSettingLang, lang.isoCode());
	}
	else {
		qAppSettings.loadLanguage(qAppSettings.valueAsString(kAppSettingLang));
		XAMP_LOG_DEBUG("Load locale language file: {}.",
			qAppSettings.valueAsString(kAppSettingLang).toStdString());
	}
}
