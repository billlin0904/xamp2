#include <QStorageInfo>
#include <widget/str_utilts.h>
#include <widget/drivewatcher.h>
#include <widget/widget_shared.h>

DriveWatcher::DriveWatcher(QObject* parent)
	: QObject(parent)
	, timer_(this) {
    file_info_list_ = QDir::drives();
    (void)QObject::connect(&timer_, &QTimer::timeout, [this]() {
        checkForDriveChanges();
        });
    timer_.setInterval(500);
    timer_.start();
}

void DriveWatcher::readDriveInfo() {
    static const QList<std::string> kCDFileSystemType = {
        "CDFS",
        "UDF",
        "ISO-9660",
        "ISO9660"
    };

    QList<DriveInfo> drives;
    Q_FOREACH(auto & storage, QStorageInfo::mountedVolumes()) {
        if (storage.isValid() && storage.isReady()) {
            auto display_name = storage.displayName() + Q_TEXT("(") + storage.rootPath() + Q_TEXT(")");
            const auto driver_letter = storage.rootPath().left(1).toStdString()[0];
            const auto file_system_type = storage.fileSystemType();
            if (kCDFileSystemType.contains(file_system_type.toUpper().toStdString())) {
                auto device = OpenCD(driver_letter);
                auto device_info = device->GetCDDeviceInfo();
                display_name += QString::fromStdWString(L" " + device_info.product);
                drives.push_back(DriveInfo { driver_letter , display_name });
            }
        }
    }
    if (drives.empty()) {
        return;
    }
    emit drivesChanges(drives);
}

void DriveWatcher::checkForDriveChanges() {	
	auto current_file_info_list = QDir::drives();
	auto drives_changed = false;

	if (current_file_info_list.size() == file_info_list_.size()) {
		drives_changed = true;
	} else {
        auto num_matches = 0;

		Q_FOREACH(QFileInfo current_file_info, current_file_info_list) {
            Q_FOREACH(QFileInfo original_file_info, file_info_list_) {
                if (current_file_info.absolutePath() == original_file_info.absolutePath()) {
                    num_matches++;
                    break;
                }
            }
        }
        drives_changed = num_matches != current_file_info_list.size();
	}

    if (drives_changed) {
        file_info_list_ = QDir::drives();
        readDriveInfo();
    }
}