#include "filesystementity.h"

FileSystemEntity::FileSystemEntity(const QFileInfo& file_info)
	: file_info_(file_info) {
}

void FileSystemEntity::setParent(std::shared_ptr<FileSystemEntity> parent) {
	parent_ = parent;

	if (auto p = parent_.lock()) {
		parent->addChild(shared_from_this());
		if (p->parent() == nullptr) {
			absolute_file_path_ = file_info_.canonicalPath();
		}
		else {
			absolute_file_path_ = parent->absoluteFilePath() + Q_UTF8("/") + file_info_.fileName();
		}
	}
}

QString FileSystemEntity::absoluteFilePath() const {
	return absolute_file_path_;
}

std::shared_ptr<FileSystemEntity> FileSystemEntity::parent() {
	return parent_.lock();
}

void FileSystemEntity::addChild(std::shared_ptr<FileSystemEntity> child) {
	children_.push_back(child);
}

QString FileSystemEntity::fileName() const {
	if (auto p = parent_.lock()) {
		if (p->parent() == nullptr) {
			return file_info_.canonicalPath();
		}
		else {
			return file_info_.fileName();
		}
	}
	return Q_EMPTY_STR;
}

std::shared_ptr<FileSystemEntity> FileSystemEntity::matchPath(const QStringList& path, int32_t startIndex) {
	for (auto& child : children_) {
		auto match = path.at(startIndex);
		if (child->fileName() == match) {
			if (startIndex + 1 == path.count()) {
				return child;
			}
			else {
				return child->matchPath(path, startIndex + 1);
			}
		}
	}
}

int FileSystemEntity::childCount() const {
	return static_cast<int>(children_.size());
}

QFileInfo FileSystemEntity::fileInfo() const {
	return file_info_;
}

FileSystemEntity* FileSystemEntity::childAt(int position) {
	return children_[position].get();
}

int FileSystemEntity::childNumber() const {
	int index = 0;
	if (auto p = parent_.lock()) {
		for (auto& child : children_) {
			if (child.get() == this) {
				break;
			}
			++index;
		}
	}
	return index;
}
