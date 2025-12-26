#include <thememanager.h>
#include <QFileIconProvider>
#include <widget/util/str_util.h>
#include <widget/filesystemmodel.h>

namespace {
	class FileFileIconProvider : public QFileIconProvider {
	public:
		QIcon icon(const QFileInfo& info) const override {
			if (isZipFile(info)) {
				return qTheme.fontIcon(Glyphs::ICON_ARCHIVE);
			}
			if (isDisk(info)) {
				return qTheme.fontIcon(Glyphs::ICON_HD);
			}
			if (info.isDir()) {
				return qTheme.fontIcon(Glyphs::ICON_FOLDER);
			}			
			return qTheme.fontIcon(Glyphs::ICON_AUDIO_FILE);
		}

	private:
		static bool isZipFile(const QFileInfo& info) {
			const auto suffix = info.suffix().toLower();
			if (suffix == "zip"_str
				|| suffix == "rar"_str
				|| suffix == "7z"_str
				|| suffix == "tar"_str
				|| suffix == "gz"_str) {
				return true;
			}
			return false;
		}

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