//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#include <spdlog/spdlog.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/sinks/sink.h>
#include <spdlog/fmt/ostr.h>

#include <QDebug>

class QDebugSink final : public spdlog::sinks::base_sink<std::mutex> {
public:
    QDebugSink() {
    }

private:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        spdlog::memory_buf_t formatted;
        formatter_->format(msg, formatted);
        auto logging = fmt::to_string(formatted);
        qDebug("%s", logging.c_str());
    }

    void flush_() override {
    }
};


