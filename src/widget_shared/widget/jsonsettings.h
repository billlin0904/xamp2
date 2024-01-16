//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QSettings>
#include <QScopedPointer>

#include <widget/str_utilts.h>
#include <widget/appsettingnames.h>
#include <widget/widget_shared_global.h>

class XAMP_WIDGET_SHARED_EXPORT JsonSettings {
public:
    static void loadJsonFile(QString const& file_name);

    static QMap<QString, QVariant> valueAsMap(QString const& key);

    static QVariant valueAs(QString const& key);

    static int32_t valueAsInt(QString const& key);

    static void remove(const QString& key);

    template <typename T>
    static void setDefaultValue(QString const& key, T const & value) {
        default_settings_[key] = value;
    }

    static void setValue(QString const & key, QVariant const & value) {
        settings_->setValue(key, value);
    }

    static bool contains(QString const& key) {
        return settings_->contains(key);
    }

    static QStringList keys();

    static void save();

protected:
    JsonSettings() = default;

private:
    static QScopedPointer<QSettings> settings_;
    static QMap<QString, QVariant> default_settings_;
};

