//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================
#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/sinks/sink.h>
#include <spdlog/fmt/ostr.h>

#include <widget/util/str_utilts.h>
#include <widget/widget_shared_global.h>

#include <QDebug>

#ifdef Q_OS_MAC
class XAMP_WIDGET_SHARED_EXPORT QDebugSink final : public spdlog::sinks::base_sink<std::mutex> {
public:
    QDebugSink() = default;

    virtual ~QDebugSink() override = default;

private:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        spdlog::memory_buf_t formatted;
        formatter_->format(msg, formatted);
        auto logging = fmt::to_string(formatted);
        qDebug().nospace().noquote() << logging.c_str();
    }

    void flush_() override {
    }
};
#endif


