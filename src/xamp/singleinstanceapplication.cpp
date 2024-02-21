#include <QFileInfo>
#include <QLocalServer>
#include <QLocalSocket>
#include <QSysInfo>

#include <base/logger_impl.h>
#include <base/platform.h>

#include <widget/xmainwindow.h>

#include <singleinstanceapplication.h>

constexpr auto kIpcTimeout = 1000;

SingleInstanceApplication::SingleInstanceApplication(int& argc, char* argv[])
	: QApplication(argc, argv) {
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
}

SingleInstanceApplication::~SingleInstanceApplication() = default;

bool SingleInstanceApplication::isAttach() const {
    return !is_running_;
}
