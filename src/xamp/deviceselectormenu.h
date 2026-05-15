//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QActionGroup>
#include <QFrame>
#include <QLabel>
#include <QScopedPointer>
#include <QToolButton>

#include <widget/widget_shared.h>
#include <output_device/iaudiodevicemanager.h>
#include <output_device/idevicetype.h>

#include <base/memory.h>
#include <base/uuid.h>

#include <functional>
#include <optional>
#include <string>

class QMenu;
class QWidgetAction;

class DeviceSelectorMenu final {
public:
    using DeviceSelectedCallback = std::function<void(const DeviceInfo&)>;

    DeviceSelectorMenu(QToolButton* select_device_button,
        QLabel* device_desc_label,
        QObject* action_parent);

    [[nodiscard]] std::optional<DeviceInfo> rebuild(
        const ScopedPtr<IAudioDeviceManager>& device_manager,
        const Uuid& preferred_device_type_id,
        const std::string& preferred_device_id,
        const std::optional<DeviceInfo>& current_device_info,
        DeviceSelectedCallback on_selected);

private:
    [[nodiscard]] QString translateDeviceDescription(const IDeviceType* device_type) const;

    [[nodiscard]] QWidgetAction* createHeaderAction(const QString& desc);

    void applySelectedDevice(const DeviceInfo& device_info, int label_width);

    [[nodiscard]] QMenu* ensureMenu() const;

    int device_label_width_{ 0 };
    QToolButton* select_device_button_;
    QLabel* device_desc_label_;
    QObject* action_parent_;
    QScopedPointer<QActionGroup> action_group_;
    QList<QFrame*> device_type_frames_;
};
