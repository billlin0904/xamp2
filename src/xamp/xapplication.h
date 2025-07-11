//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QApplication>
#include <QScopedPointer>
#include <base/logger_impl.h>

#include <widget/widget_shared_global.h>

class XMainWindow;
class QLocalServer;

class XApplication : public QApplication {
public:
    XApplication(int& argc, char* argv[]);

	virtual ~XApplication() override;

    [[nodiscard]] bool isAttach() const;

    void initial();

    void loadLang();

    void setTheme();

    void loadSampleRateConverterConfig();
private:
    bool is_running_{ false };
    XMainWindow* window_{ nullptr };
    QScopedPointer<QLocalServer> server_;
    LoggerPtr logger_;
};
