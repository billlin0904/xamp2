//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#include <deviceselectormenu.h>

#include <thememanager.h>

#include <QCoreApplication>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QMap>
#include <QMenu>
#include <QWidgetAction>

#include <output_device/api.h>
#include <output_device/idevicetype.h>
#include <widget/actionmap.h>
#include <widget/util/str_util.h>

#include <algorithm>

namespace {
    template <typename Map>
    QString translateLookup(const char* context,
        const Map& lut,
        const std::string_view key,
        const QString& fallback) {
        const auto source_text = lut.value(key, nullptr);
        if (source_text == nullptr) {
            return fallback;
        }
        return QCoreApplication::translate(context, source_text);
    }
}

DeviceSelectorMenu::DeviceSelectorMenu(QToolButton* select_device_button,
    QLabel* device_desc_label,
    QObject* action_parent)
    : select_device_button_(select_device_button)
    , device_desc_label_(device_desc_label)
    , action_parent_(action_parent) {
    Q_ASSERT(select_device_button_ != nullptr);
    Q_ASSERT(device_desc_label_ != nullptr);
    Q_ASSERT(action_parent_ != nullptr);
}

std::optional<DeviceInfo> DeviceSelectorMenu::rebuild(
    const ScopedPtr<IAudioDeviceManager>& device_manager,
    const Uuid& preferred_device_type_id,
    const std::string& preferred_device_id,
    const std::optional<DeviceInfo>& current_device_info,
    DeviceSelectedCallback on_selected) {
    auto f = qTheme.defaultFont();
    f.setPointSize(10);
    device_desc_label_->setFont(f);

    auto* menu = ensureMenu();
    menu->clear();
    device_type_frames_.clear();

    action_group_.reset(new QActionGroup(action_parent_));
    action_group_->setExclusive(true);

    OrderedMap<std::string, QAction*> device_id_action;
    std::optional<DeviceInfo> selected_device_info;
    std::optional<DeviceInfo> default_device_info;

    const QFontMetrics metrics(device_desc_label_->font());
    auto max_width = current_device_info.has_value()
        ? metrics.horizontalAdvance(QString::fromStdWString(current_device_info->name))
        : 0;

    for (auto itr = device_manager->Begin(); itr != device_manager->End(); ++itr) {
        const auto device_type = itr->second();
        device_type->ScanNewDevice();

        const auto device_info_list = device_type->GetDeviceInfo();
        if (device_info_list.empty()) {
            continue;
        }

        menu->addSeparator();
        menu->addAction(createHeaderAction(translateDeviceDescription(device_type.get())));

        for (const auto& device_info : device_info_list) {
            const auto device_name = QString::fromStdWString(device_info.name);
            max_width = (std::max)(metrics.horizontalAdvance(device_name), max_width);

            auto* device_action = new QAction(qTheme.connectTypeIcon(device_info.connect_type), device_name, menu);
            action_group_->addAction(device_action);
            device_action->setCheckable(true);
            device_action->setChecked(false);
            device_action->setProperty("deviceName", device_name);

            const auto update_device_action = [device_action]() {
                const auto name = device_action->property("deviceName").toString();
                device_action->setText(device_action->isChecked()
                    ? QStringLiteral("%1 %2").arg(QChar(0x2713), name)
                    : name);
            };
            (void)QObject::connect(device_action, &QAction::toggled, action_parent_, update_device_action);

            const auto select_device = [this, device_info, on_selected]() {
                applySelectedDevice(device_info, device_label_width_);
                if (on_selected) {
                    on_selected(device_info);
                }
            };
            (void)QObject::connect(device_action, &QAction::triggered, action_parent_, select_device);

            device_id_action[device_info.device_id] = device_action;
            menu->addAction(device_action);

            if (preferred_device_type_id == device_info.device_type_id
                && preferred_device_id == device_info.device_id) {
                selected_device_info = device_info;
                device_action->setChecked(true);
            }

            if (!default_device_info.has_value()
                && device_info.is_default_device
                && !IsExclusiveDevice(device_info)) {
                default_device_info = device_info;
            }
        }
    }

    if (!selected_device_info.has_value()) {
        selected_device_info = default_device_info;
        if (selected_device_info.has_value()) {
            if (const auto itr = device_id_action.find(selected_device_info->device_id);
                itr != device_id_action.end()) {
                itr->second->setChecked(true);
            }
        }
    }

    device_label_width_ = max_width;
    if (selected_device_info.has_value()) {
        applySelectedDevice(*selected_device_info, device_label_width_);
    }

    return selected_device_info;
}

QString DeviceSelectorMenu::translateDeviceDescription(const IDeviceType* device_type) const {
    static const QMap<std::string_view, const char*> lut{
        { "WASAPI (Exclusive Mode)", QT_TRANSLATE_NOOP("Xamp", "WASAPI (Exclusive Mode)") },
        { "WASAPI (Shared Mode)",    QT_TRANSLATE_NOOP("Xamp", "WASAPI (Shared Mode)") },
        { "Null Output",             QT_TRANSLATE_NOOP("Xamp", "Null Output") },
        { "XAudio2",                 QT_TRANSLATE_NOOP("Xamp", "XAudio2") },
        { "ASIO",                    QT_TRANSLATE_NOOP("Xamp", "ASIO") },
    };
    return translateLookup("Xamp",
        lut,
        device_type->GetDescription(),
        fromStdStringView(device_type->GetDescription()));
}

QWidgetAction* DeviceSelectorMenu::createHeaderAction(const QString& desc) {
    auto* desc_label = new QLabel(desc);
    desc_label->setObjectName("textSeparator"_str);

    QFont f = desc_label->font();
    f.setPointSize(qTheme.fontSize(9));
    f.setBold(true);
    desc_label->setFont(f);
    desc_label->setAlignment(Qt::AlignCenter);

    auto* device_type_frame = new QFrame();
    device_type_frame->setObjectName("deviceTypeFrame"_str);
    qTheme.setFrameBackgroundColor(device_type_frame);

    auto* default_layout = new QHBoxLayout(device_type_frame);
    default_layout->addWidget(desc_label);
    default_layout->setSpacing(0);
    default_layout->setContentsMargins(0, 0, 0, 0);
    device_type_frame->setLayout(default_layout);

    auto* separator = new QWidgetAction(ensureMenu());
    separator->setDefaultWidget(device_type_frame);

    device_type_frames_.push_back(device_type_frame);
    return separator;
}

void DeviceSelectorMenu::applySelectedDevice(const DeviceInfo& device_info, int label_width) {
    qTheme.setDeviceConnectTypeIcon(select_device_button_, device_info.connect_type);
    device_desc_label_->setMinimumWidth(label_width + 60);
    device_desc_label_->setText(QString::fromStdWString(device_info.name));
}

QMenu* DeviceSelectorMenu::ensureMenu() const {
    auto* menu = select_device_button_->menu();
    if (!menu) {
        menu = new XMenu();
        select_device_button_->setMenu(menu);
    }
    return menu;
}
