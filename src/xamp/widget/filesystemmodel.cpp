#include "filesystemmodel.h"

FileSystemModel::FileSystemModel(QObject* parent)
	: QAbstractItemModel(parent) {
	headers_ << Q_UTF8("Name")
		<< Q_UTF8("Size")
		<< Q_UTF8("Type")
		<< Q_UTF8("Date Modified");
	root_ = std::make_shared<FileSystemEntity>(QFileInfo());
	for (const auto& drive : QDir::drives()) {
		auto child = std::make_shared<FileSystemEntity>(drive);
		child->setParent(root_);
		root_->addChild(child);
	}
}

QVariant FileSystemModel::headerData(int section, Qt::Orientation orientation, int role) const {
	if (orientation == Qt::Horizontal) {
		switch (role) {
		case Qt::DisplayRole:
			return headers_.at(section);
		case Qt::TextAlignmentRole:
			return section == COLUMN_SIZE ? Qt::AlignRight : Qt::AlignLeft;
		}
	}
	return QVariant();
}

Qt::ItemFlags FileSystemModel::flags(const QModelIndex& index) const {
	if (!index.isValid()) {
		return 0;
	}
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

int FileSystemModel::columnCount(const QModelIndex&) const {
	return _MAX_COLUMN_;
}

int FileSystemModel::rowCount(const QModelIndex& parent) const {
	return getItem(parent)->childCount();
}

QVariant FileSystemModel::data(const QModelIndex& index, int role) const {
	if (!index.isValid()) {
		return QVariant();
	}

	if (index.column() == _MAX_COLUMN_ && Qt::TextAlignmentRole == role) {
		return Qt::AlignRight;
	}

	if (role != Qt::DisplayRole && role != Qt::DecorationRole) {
		return QVariant();
	}

	auto entity = getItem(index);
	if (!entity) {
		return QVariant();
	}

	if (role == Qt::DecorationRole && index.column() == COLUMN_NAME) {
		return icon_provider_.icon(entity->fileInfo());
	}

	QVariant data = QString();
	auto col = static_cast<Column>(index.column());
	switch (col) {
	case COLUMN_NAME:
		data = entity->fileName();
		break;
	case COLUMN_SIZE:
		if (entity->fileInfo().isDir()) {
			data = QString();
		}
		else {
			data = entity->fileInfo().size();
		}
		break;
	case COLUMN_TYPE:
		data = icon_provider_.type(entity->fileInfo());
		break;
	case COLUMN_DATE:
		data = entity->fileInfo().lastModified().toString(Qt::LocalDate);
		break;
	}
	return data;
}

QModelIndex FileSystemModel::index(int row, int column, const QModelIndex& parent) const {
	if (parent.isValid() && parent.column() != COLUMN_NAME) {
		return QModelIndex();
	}

	auto parent_entity = getItem(parent);
	if (parent_entity) {
		auto child_entity = parent_entity->childAt(row);
		if (child_entity) {
			return createIndex(row, column, child_entity);
		}
	}

	return QModelIndex();
}

QModelIndex FileSystemModel::index(const QString& path, int column) const {
	if (path.length() > 0) {
		auto item = root_->matchPath(path.split(Q_UTF8("/")));
		if (item) {
			return createIndex(item->childNumber(), column, item.get());
		}
	}
	return QModelIndex();
}

bool FileSystemModel::isDir(const QModelIndex& index) const {
	const auto item = static_cast<FileSystemEntity*>(index.internalPointer());
	if (item && item != root_.get()) {
		return item->fileInfo().isDir();
	}
	return false;
}

QString FileSystemModel::absolutePath(const QModelIndex& index) const {
	const auto item = static_cast<FileSystemEntity*>(index.internalPointer());
	if (item && item != root_.get()) {
		return item->absoluteFilePath();
	}
	return Q_EMPTY_STR;
}

QString FileSystemModel::currentPath() const {
	return current_path_;
}

QModelIndex FileSystemModel::setCurrentPath(const QString& path) {
	current_path_ = path;
	auto item = root_->matchPath(path.split(Q_UTF8("/")));
	if (item && item != root_ && item->childCount() == 0) {
		populateItem(item.get());
	}
	return index(path, 0);
}

void FileSystemModel::populateItem(FileSystemEntity* item) {
	QDir dir(item->absoluteFilePath());
	auto all = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);
	for (const auto& info : all) {
		item->addChild(std::make_shared<FileSystemEntity>(info));
	}
}

FileSystemEntity* FileSystemModel::getItem(const QModelIndex& index) const {
	if (index.isValid()) {
		auto item = static_cast<FileSystemEntity*>(index.internalPointer());
		if (item) {
			return item;
		}
	}
	return root_.get();
}