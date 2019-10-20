#include "xamp.h"

#include <base/logger.h>
#include <player/audio_player.h>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/sinks/sink.h>
#include <spdlog/fmt/ostr.h>

#include "widget/appsettings.h"
#include "widget/database.h"

#include <QDebug>
#include <QtWidgets/QApplication>

class QDebugSink final : public spdlog::sinks::base_sink<std::mutex> {
public:
    QDebugSink() {
    }

private:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        spdlog::memory_buf_t formatted;
        formatter_->format(msg, formatted);
        qErrnoWarning(fmt::to_string(formatted).c_str());
    }

    void flush_() override {
    }
};

static void loadAndDefaultAppConfig() {
	AppSettings::settings().loadIniFile(Q_UTF8("xamp.ini"));
	AppSettings::settings().setDefaultValue(APP_SETTING_DEVICE_TYPE, Q_UTF8(""));
	AppSettings::settings().setDefaultValue(APP_SETTING_DEVICE_ID, Q_UTF8(""));
	AppSettings::settings().setDefaultValue(APP_SETTING_WIDTH, 600);
	AppSettings::settings().setDefaultValue(APP_SETTING_HEIGHT, 500);
	AppSettings::settings().setDefaultValue(APP_SETTING_VOLUME, 50);
	AppSettings::settings().setDefaultValue(APP_SETTING_NIGHT_MODE, false);
	AppSettings::settings().setDefaultValue(APP_SETTING_ORDER, PlayerOrder::PLAYER_ORDER_REPEAT_ONCE);
}

int main(int argc, char *argv[]) {
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

	Logger::Instance()
#ifdef _DEBUG
		.AddDebugOutputLogger()
#endif
#ifdef Q_OS_MAC
        .AddSink(std::make_shared<QDebugSink>())
#endif
		.AddFileLogger("xamp.log");	

	QApplication app(argc, argv);

	try {
		Database::Instance().open(Q_UTF8("xamp.db"));
	}
	catch (const std::exception& e) {
		XAMP_LOG_INFO("Initial database failure! {}", e.what());
		return -1;
	}

	loadAndDefaultAppConfig();

    Xamp win;
    win.show();
	return app.exec();
}
