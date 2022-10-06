#include <QWinEventNotifier>
#include <QSettings>

#include <base/logger_impl.h>
#include <base/exception.h>
#include <base/platfrom_handle.h>
#include <widget/str_utilts.h>
#include <widget/win32/themenotifier.h>

class ThemeNotifier::ThemeNotifierImpl {
public:
#ifdef Q_OS_WIN
	explicit ThemeNotifierImpl(ThemeNotifier* notifier) {
		notifier_.reset(new QWinEventNotifier());

		(void)QObject::connect(notifier_.get(), &QWinEventNotifier::activated, [notifier]() {
			const auto appsUseLightTheme = QSettings{
			Q_TEXT("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"),
			QSettings::NativeFormat }.value(Q_TEXT("AppsUseLightTheme")).toBool();
			emit notifier->themeChanged(appsUseLightTheme ? ThemeColor::LIGHT_THEME : ThemeColor::DARK_THEME);
			});

		constexpr DWORD filter = REG_NOTIFY_CHANGE_NAME |
			REG_NOTIFY_CHANGE_ATTRIBUTES |
			REG_NOTIFY_CHANGE_LAST_SET |
			REG_NOTIFY_CHANGE_SECURITY;

		HKEY key = nullptr;
		auto result = ::RegOpenKeyExW(HKEY_CURRENT_USER,
			L"HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
			0,
			KEY_NOTIFY,
			&key);
		if (result != ERROR_SUCCESS) {
			XAMP_LOG_DEBUG("RegOpenKeyExW return failure! {}", GetPlatformErrorMessage(result));
			return;
		}

		key_.reset(key);

		event_.reset(::CreateEvent(nullptr, TRUE, FALSE, nullptr));
		if (!event_) {
			XAMP_LOG_DEBUG("CreateEvent return failure! {}", GetLastErrorMessage());
			return;
		}

		result = ::RegNotifyChangeKeyValue(key_.get(),
			TRUE,
			filter,
			event_.get(),
			TRUE);
		if (result != ERROR_SUCCESS) {
			XAMP_LOG_DEBUG("RegNotifyChangeKeyValue return failure! {}", GetPlatformErrorMessage(result));
			return;
		}

		notifier_->setHandle(event_.get());
		notifier_->setEnabled(true);
	}

	RegHandle key_;
	WinHandle event_;
	QScopedPointer<QWinEventNotifier> notifier_;
#else
	explicit ThemeNotifierImpl(ThemeNotifier* notifier) {
	}
#endif
};

ThemeNotifier::ThemeNotifier(QObject* parent)
	: QObject(parent)
	, impl_(new ThemeNotifierImpl(this)) {
}

ThemeNotifier::~ThemeNotifier() = default;
