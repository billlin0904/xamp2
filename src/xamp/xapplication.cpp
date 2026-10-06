#include <QFileInfo>
#include <QLocalServer>
#include <QLocalSocket>
#include <QSysInfo>

#include <base/logger.h>
#include <base/platform.h>
#include <base/crashhandler.h>
#include <base/platform.h>
#include <base/dll.h>

#include <widget/database.h>
#include <widget/xmainwindow.h>
#include <widget/appsettings.h>
#include <widget/jsonsettings.h>
#include <widget/util/ui_util.h>

#include <xapplication.h>

#include <base/scopeguard.h>
#include <metadata/imetadatascanreader.h>

#ifdef Q_OS_WIN
#include <mimalloc.h>
#include <Windows.h>
#include <shellapi.h>
#pragma comment(lib, "Shell32.lib")
#endif

constexpr auto kIpcTimeout = 1000;

#ifdef Q_OS_WIN
std::wstring quoteArgument(const std::wstring& argument) {
    std::wstring result = L"\"";
    size_t slashes = 0;
    for (const auto ch : argument) {
        if (ch == L'\\') {
            ++slashes;
            continue;
        }
        result.append(ch == L'"' ? slashes * 2 + 1 : slashes, L'\\');
        result += ch;
        slashes = 0;
    }
    result.append(slashes * 2, L'\\');
    result += L'"';
    return result;
}

// Choose before acquiring the single-instance server or loading components.
bool XApplication::selectStartupScanMode() {
    if (xamp::base::isRunAsAdmin()) {
        xamp::metadata::setNtfsMetadataScanEnabled(true);
        return true;
    }

    constexpr auto kNtfsChild = L"--ntfs-elevated-child";
    int argument_count = 0;
    auto** arguments = ::CommandLineToArgvW(::GetCommandLineW(), &argument_count);
    if (!arguments) {
        return true;
    }

    XAMP_ON_SCOPE_EXIT([&] {
        ::LocalFree(arguments);
        });
    bool elevated_child = false;
    std::wstring parameters;
    for (int i = 1; i < argument_count; ++i) {
        if (std::wstring_view(arguments[i]) == kNtfsChild) {
            elevated_child = true;
        }
        else {
            parameters += quoteArgument(arguments[i]) + L" ";
        }
    }

    if (elevated_child) {
        xamp::metadata::setNtfsMetadataScanEnabled(false);
        ::MessageBoxW(nullptr, L"Administrator access was not obtained. Standard scanning will be used.",
            L"xamp", MB_OK | MB_ICONINFORMATION);
        return true;
    }

    const auto choice = ::MessageBoxW(nullptr,
        L"Enable NTFS scanning for this session?\n\n"
        L"Yes: Use administrator privileges for direct NTFS reads. "
        L"Other file systems use standard scanning.\n"
        L"No: Use standard scanning without requesting administrator privileges.\n"
        L"Cancel: Exit xamp.",
        L"xamp - Scan mode", MB_YESNOCANCEL | MB_ICONQUESTION | MB_DEFBUTTON2);
    if (choice == IDCANCEL) {
        return false;
    }
    if (choice != IDYES) {
        return true;
    }
    std::wstring executable(32768, L'\0');
    const auto length = ::GetModuleFileNameW(nullptr, executable.data(),
        static_cast<DWORD>(executable.size()));
    if (length != 0 && length < executable.size()) {
        executable.resize(length);
        parameters += kNtfsChild;
        SHELLEXECUTEINFOW info{};
        info.cbSize = sizeof(info);
        info.fMask = SEE_MASK_NOASYNC | SEE_MASK_FLAG_NO_UI;
        info.lpVerb = L"runas";
        info.lpFile = executable.c_str();
        info.lpParameters = parameters.c_str();
        info.nShow = SW_SHOWNORMAL;
        if (::ShellExecuteExW(&info)) {
            return false;
        }
    }
    ::MessageBoxW(nullptr, L"Administrator startup was canceled or failed. Standard scanning will be used.",
        L"xamp", MB_OK | MB_ICONINFORMATION);
    return true;
}
#else
bool XApplication::selectStartupScanMode() {
	return true;
}
#endif

XAMP_DECLARE_LOG_NAME(XApplication);

XApplication::XApplication(int& argc, char* argv[])
	: QApplication(argc, argv) {
	logger_ = XAMP_LOG_CREATE_LOGGER(XApplication);

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
	
	//setCurrentProcessPriority(ProcessPriority::PRIORITY_BACKGROUND);
}

XApplication::~XApplication() = default;

bool XApplication::isAttach() const {
    return !is_running_;
}

void XApplication::initial() {
	const auto app_path = applicationPath();
	qAppSettings.loadIniFile(app_path + "/xamp.ini"_str);
	qJsonSettings.loadJsonFile(app_path + "/config.json"_str);

	qAppSettings.loadOrSaveLogConfig();
	qAppSettings.loadAppSettings();
}

void XApplication::loadSampleRateConverterConfig() {
	XAMP_LOG_DEBUG("LoadSampleRateConverterConfig.");
	qAppSettings.loadSoxrSetting();
	qAppSettings.LoadR8BrainSetting();
	qJsonSettings.save();
	XAMP_LOG_DEBUG("loadLogAndSoxrConfig success.");
}

void XApplication::setTheme() {
	qTheme.setThemeQssFile();
	const auto theme = qAppSettings.valueAsEnum<ThemeColor>(kAppSettingTheme);
	qTheme.setThemeColor(theme);
}

void XApplication::loadLang() {
	XAMP_LOG_DEBUG("load language file.");

	if (qAppSettings.valueAsString(kAppSettingLang).isEmpty()) {
		const LocaleLanguage lang;
		XAMP_LOG_DEBUG("load locale language file: {}.", lang.isoCode().toStdString());
		qAppSettings.loadLanguage(lang.isoCode());
		qAppSettings.loadLanguage(qFormat("qt_%1").arg(lang.isoCode()));
		qAppSettings.setValue(kAppSettingLang, lang.isoCode());
	}
	else {
		qAppSettings.loadLanguage(qAppSettings.valueAsString(kAppSettingLang));
		XAMP_LOG_DEBUG("load locale language file: {}.",
			qAppSettings.valueAsString(kAppSettingLang).toStdString());
	}
}
