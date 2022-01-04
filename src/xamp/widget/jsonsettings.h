//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QSettings>
#include <QScopedPointer>

#include <widget/str_utilts.h>
#include <widget/settingnames.h>

class JsonSettings {
public:
    static void loadJsonFile(QString const& file_name);

    static QVariant getValue(QString const& key);

    static int32_t getAsInt(QString const& key);

    static void remove(const QString& key);

    template <typename T>
    static void setDefaultValue(QString const& key, T const & value) {
        default_settings_[key] = value;
    }

    static void setValue(QString const & key, QVariant const & value) {
        settings_->setValue(key, value);
    }

    static QStringList keys();

    static void save();

protected:
    JsonSettings() = default;

private:
    static QScopedPointer<QSettings> settings_;
    static QMap<QString, QVariant> default_settings_;
};

