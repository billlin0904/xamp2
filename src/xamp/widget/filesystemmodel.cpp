#include <thememanager.h>
#include <widget/filesystemmodel.h>

FileSystemModel::FileSystemModel(QObject* parent)
	: QFileSystemModel(parent) {
}

QVariant FileSystemModel::data(const QModelIndex& index, int role) const {
	if (role == Qt::DecorationRole) {
		return qTheme.folderIcon();
	}
	return QFileSystemModel::data(index, role);
}