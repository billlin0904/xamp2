//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QApplication>
#include <QScopedPointer>

class XMainWindow;
class QLocalServer;

class SingleInstanceApplication : public QApplication {
    Q_OBJECT
public:
    static constexpr int kReduceWorkingSetSeconds = 1 * 1000;

    SingleInstanceApplication(int& argc, char* argv[]);

	virtual ~SingleInstanceApplication() override;

    bool IsAttach() const;

    XMainWindow* window{ nullptr };

private:
    bool is_running_{ false };
    QScopedPointer<QLocalServer> server_;
};
