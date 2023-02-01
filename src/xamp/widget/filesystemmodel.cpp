#include <thememanager.h>
#include <widget/filesystemmodel.h>

FileSystemModel::FileSystemModel(QObject* parent)
	: QFileSystemModel(parent) {
}

QVariant FileSystemModel::data(const QModelIndex& index, int role) const {
	if (role == Qt::DecorationRole) {
		if (isDir(index)) {
			return qTheme.GetFontIcon(Glyphs::ICON_FOLDER);
		}
		return qTheme.GetFontIcon(Glyphs::ICON_AUDIO);
	} else if (role == Qt::ToolTipRole) {
		return filePath(index);
	}
	return QFileSystemModel::data(index, role);
}