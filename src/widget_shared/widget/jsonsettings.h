//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QSettings>
#include <QScopedPointer>

#include <widget/util/str_util.h>
#include <widget/appsettingnames.h>
#include <widget/widget_shared_global.h>

class XAMP_WIDGET_SHARED_EXPORT JsonSettings {
public:
    JsonSettings() = default;

    void loadJsonFile(QString const& file_name);

    QMap<QString, QVariant> valueAsMap(QString const& key);

    QVariant valueAs(QString const& key) const;

	int32_t valueAsInt(QString const& key) const;

    void remove(const QString& key) const;

    template <typename T>
    void setDefaultValue(QString const& key, T const & value) {
        default_settings_[key] = value;
    }

    void setValue(QString const & key, QVariant const & value) {
        settings_->setValue(key, value);
    }

    bool contains(QString const& key) {
        return settings_->contains(key);
    }

    QStringList keys() const;

    void save() const;

private:
    QScopedPointer<QSettings> settings_;
    QMap<QString, QVariant> default_settings_;
};

#define qJsonSettings SharedSingleton<JsonSettings>::GetInstance()
