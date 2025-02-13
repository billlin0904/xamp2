#include <thememanager.h>
#include <QFileIconProvider>
#include <widget/filesystemmodel.h>

namespace {
	class FileFileIconProvider : public QFileIconProvider {
	public:
		QIcon icon(const QFileInfo& info) const override {
			if (isDisk(info)) {
				return qTheme.fontIcon(Glyphs::ICON_HD);
			}
			if (info.isDir()) {
				return qTheme.fontIcon(Glyphs::ICON_FOLDER);
			}
			return qTheme.fontIcon(Glyphs::ICON_AUDIO_FILE);
		}

	private:
		static bool isDisk(const QFileInfo& info) {
			const auto path = 
				QDir::toNativeSeparators(info.absoluteFilePath());

			if (info.isDir() 
				&& path.size() == 3 
				&& path[1] == QLatin1Char(':') 
				&& path[2] == QLatin1Char('\\')) {
				return true;
			}
			return false;
		}
	}; 
}

FileSystemModel::FileSystemModel(QObject* parent)
	: QFileSystemModel(parent) {
	setIconProvider(new FileFileIconProvider());
}

QVariant FileSystemModel::data(const QModelIndex& index, int role) const {
	return QFileSystemModel::data(index, role);
}