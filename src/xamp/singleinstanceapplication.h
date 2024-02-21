//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QApplication>
#include <QScopedPointer>

class XMainWindow;
class QLocalServer;

class SingleInstanceApplication : public QApplication {
public:
    SingleInstanceApplication(int& argc, char* argv[]);

	virtual ~SingleInstanceApplication() override;

    [[nodiscard]] bool isAttach() const;

private:
    bool is_running_{ false };
    XMainWindow* window_{ nullptr };
    QScopedPointer<QLocalServer> server_;
};
