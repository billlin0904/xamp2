#include <QCloseEvent>
#include <QCryptographicHash>

#include <ui_tageditpage.h>

#include <widget/databasefacade.h>
#include <widget/util/str_util.h>
#include <widget/dao/albumdao.h>
#include <widget/dao/artistdao.h>
#include <widget/util/image_util.h>
#include <widget/util/ui_util.h>
#include <widget/tageditpage.h>
#include <widget/xmessagebox.h>
#include <widget/tagio.h>
#include <widget/imagecache.h>
#include <widget/dao/dbfacade.h>

namespace {
	QString getFileChecksum(const QString& file_name, 
		const QCryptographicHash::Algorithm hash_algorithm) {
		QFile f(file_name);
		if (f.open(QFile::ReadOnly)) {
			QCryptographicHash hash(hash_algorithm);
			if (hash.addData(&f)) {
				return QString::fromStdString(hash.result().toHex().toUpper().toStdString());
			}
		}
		return {};
	}
}

TagEditPage::TagEditPage(QWidget* parent, const QList<PlayListEntity>& entities)
	: QFrame(parent)
	, entities_(entities) {
	ui_ = new Ui::TagEditPage();
	ui_->setupUi(this);

	const auto labels = std::vector<QLabel*>{
		ui_->titleLabel,
		ui_->artistLabel,
		ui_->audioMd5Label,
		ui_->trackPeakLabel,
		ui_->trackReplayGainLabel,
		ui_->albumPeakLabel,
		ui_->trackLabel,
		ui_->genreLabel,
		ui_->commentLabel,
		ui_->albumLabel,
		ui_->yearLabel,
		ui_->albumReplayGainLabel,
		ui_->coverSizeLabel,
		ui_->filePathLabel,
	};

	auto font = qTheme.formatFont();
	font.setPointSize(qTheme.fontSize(9));

	ui_->coverSizeLabel->setFont(font);
	ui_->albumPeakLineEdit->setFont(font);
	ui_->albumReplayGainLineEdit->setFont(font);
	ui_->trackPeakLineEdit->setFont(font);
	ui_->trackReplayGainLineEdit->setFont(font);
	ui_->audioMD5LineEdit->setFont(font);
	ui_->fileSizeLineEdit->setFont(font);

	for (auto& l : labels) {
		l->setStyleSheet("background-color: transparent;"_str);
	}

	const TagIO tag_io;

	const QStringList genre_list{
		""_str,
		"Blues"_str,
		"Classic Rock"_str,
		"Country"_str,
		"Dance"_str,
		"Disco"_str,
		"Funk"_str,
		"Grunge"_str,
		"Hip-Hop"_str,
		"Jazz"_str,
		"Metal"_str,
		"New Age"_str,
		"Oldies"_str,
		"Other"_str,
		"Pop"_str,
		"R&B"_str,
		"Rap"_str,
		"Reggae"_str,
		"Rock"_str,
		"Techno"_str,
		"Industrial"_str,
		"Alternative"_str,
		"Ska"_str,
		"Death Metal"_str,
		"Pranks"_str,
		"Soundtrack"_str,
		"Euro-Techno"_str,
		"Ambient"_str,
		"Trip-Hop"_str,
		"Vocal"_str,
	};

	ui_->genreComboBox->addItems(genre_list);

    std::sort(entities_.begin(), entities_.end(), [](auto a, auto b) {
		return a.track < b.track;
		});

	Q_FOREACH (const auto entity, entities_) {
		ui_->titleComboBox->addItem(entity.title);
	}

	Q_FOREACH(const auto entity, entities_) {
		ui_->trackComboBox->addItem(QString::number(entity.track));
	}

	(void)QObject::connect(ui_->buttonBox, &QDialogButtonBox::accepted, [this]() {
		if (XMessageBox::showYesOrNo(tr("Do you want write tag?")) != QDialogButtonBox::Yes) {
			return;
		}

		const auto index = ui_->titleComboBox->currentIndex();

		auto& entity = entities_[index];
		const Path path = entity.file_path.toStdWString();

		if (!entity.is_cue_file) {
			try {
				TagIO tag_io;
				tag_io.Open(path);
				tag_io.writeArtist(ui_->artistLineEdit->text());
				tag_io.writeTitle(ui_->titleComboBox->currentText());
				tag_io.writeAlbum(ui_->albumLineEdit->text());
				tag_io.writeComment(ui_->commentLineEdit->text());
				tag_io.writeGenre(ui_->genreComboBox->currentText());
				tag_io.writeTrack(ui_->trackComboBox->currentText().toUInt());
				tag_io.writeYear(ui_->yearLineEdit->text().toUInt());

				entity.artist = ui_->artistLineEdit->text();
				entity.title = ui_->titleComboBox->currentText();
				entity.album = ui_->albumLineEdit->text();
				entity.comment = ui_->commentLineEdit->text();
				entity.track = ui_->trackComboBox->currentText().toUInt();
				entity.year = ui_->yearLineEdit->text().toUInt();
			}
			catch (...) {
				XMessageBox::showError(tr("Failure to write tag!"));
				return;
			}
		}

		try {
			qDaoFacade.album_dao.updateAlbum(entity.album_id, entity.album);
			qDaoFacade.artist_dao.updateArtist(entity.artist_id, entity.artist);
		}
		catch (...) {
			XMessageBox::showError(tr("Failure to update database!"));
			return;
		}

		ui_->titleComboBox->setItemData(index, QBrush(Qt::green), Qt::ForegroundRole);

		const auto next_index = (index + 1) % entities_.size();
		ui_->titleComboBox->setCurrentIndex(next_index);
		ui_->trackComboBox->setCurrentIndex(next_index);
		setCurrentInfo(next_index);

		XMessageBox::showInformation(tr("Write tag successfully!"));
	});

	(void)QObject::connect(ui_->titleComboBox, &QComboBox::activated, [this](auto index) {
		setCurrentInfo(index);
		readEmbeddedCover(entities_[index]);
		});

	(void)QObject::connect(ui_->trackComboBox, &QComboBox::activated, [this](auto index) {
		setCurrentInfo(index);
		readEmbeddedCover(entities_[index]);
		});

	(void)QObject::connect(ui_->clearCommentButton, &QPushButton::clicked, [this] {
		ui_->commentLineEdit->setText(kEmptyString);
		});

	(void)QObject::connect(ui_->removeCoverButton, &QPushButton::clicked, [this] {
		const auto index = ui_->titleComboBox->currentIndex();
		TagIO tag_io;
		tag_io.Open(entities_[index].file_path.toStdWString());
		tag_io.removeEmbeddedCover();
		});

	(void)QObject::connect(ui_->saveToFileButton, &QPushButton::clicked, [this] {
		if (XMessageBox::showYesOrNo(tr("Do you want write cover image?")) != QDialogButtonBox::Yes) {
			return;
		}

		if (temp_image_.isNull()) {
			return;
		}

		if (!QFileInfo(temp_file_path_).exists()) {
			return;
		}

		const auto index = ui_->titleComboBox->currentIndex();

		TagIO tag_io;
		tag_io.Open(entities_[index].file_path.toStdWString());
		if (!tag_io.canWriteEmbeddedCover()) {
			return;
		}

		tag_io.writeEmbeddedCover(temp_image_);
		});

	(void)QObject::connect(ui_->addImageFileButton, &QPushButton::clicked, [=] {
		const auto index = ui_->titleComboBox->currentIndex();
		const auto dir = entities_[index].parent_path;
		
		const auto action = [this, index](auto file_path) {
			const auto image = image_util::readFileImage(file_path,
				kCoverSize,
				ImageCache::kImageFormat);
			if (image.isNull()) {
				return;
			}
			
			auto temp_image = image;
			const auto std_temp_file_path = GetTempFileNamePath();
			const auto temp_file_path = QString::fromStdWString(std_temp_file_path.wstring());
			if (!temp_image.save(temp_file_path, "JPG")) {
				return;
			}

			try {
				Fs::remove(temp_file_path_.toStdWString());
			}
			catch (...) {}

			temp_image_ = temp_image;
			temp_file_path_ = temp_file_path;

			const auto image_file_size = QFileInfo(temp_file_path_).size();
			const auto image_size = temp_image_.size();
			const auto resize_image = image_util::resizeImage(temp_image_, ui_->coverLabel->size());

			setImageLabel(resize_image, image_size, image_file_size);
			update();
			};

		getOpenFileName(parent,
			action,
			tr("Open image file"),
			dir,
			tr("Image Files ") + kCoverImageFileExt);
		});

	ui_->notFoundImageLabel->hide();
	ui_->trackComboBox->setCurrentIndex(0);
	ui_->titleComboBox->setCurrentIndex(0);

	setCurrentInfo(0);

	ui_->coverSizeLabel->setText(stringFormat("{} x {} (0 B)", 0, 0));
	ui_->yearLineEdit->setValidator(new QIntValidator(1, 9999));

	readEmbeddedCover(entities_[0]);

	QFontMetrics metrics(ui_->filePathLineEdit->font());
	auto path_pixel_size = metrics.horizontalAdvance(ui_->filePathLineEdit->text());
	setMinimumWidth(path_pixel_size + 100);
}

TagEditPage::~TagEditPage() {
	delete ui_;
}

void TagEditPage::setCurrentInfo(int32_t index) {
	ui_->filePathLineEdit->setText(entities_[index].file_path);
	ui_->artistLineEdit->setText(entities_[index].artist);
	ui_->albumLineEdit->setText(entities_[index].album);
	ui_->commentLineEdit->setText(entities_[index].comment);
	ui_->yearLineEdit->setText(QString::number(entities_[index].year));

	auto set_replay_gain = [&](auto* editor, const auto& opt_value) {
		if (opt_value) {
			editor->setText(formatDb(opt_value));
		}
		else {
			editor->setText(tr("Not set"));
		}
		};

	set_replay_gain(ui_->albumPeakLineEdit, entities_[index].replay_gain.value().album_peak);
	set_replay_gain(ui_->albumReplayGainLineEdit, entities_[index].replay_gain.value().album_gain);
	set_replay_gain(ui_->trackPeakLineEdit, entities_[index].replay_gain.value().track_peak);
	set_replay_gain(ui_->trackReplayGainLineEdit, entities_[index].replay_gain.value().track_gain);

	ui_->fileSizeLineEdit->setText(stringFormat("{} ({})",
		entities_[index].file_size,
		String::FormatBytes(entities_[index].file_size)));
	ui_->titleComboBox->setCurrentIndex(index);
	ui_->trackComboBox->setCurrentIndex(index);

	if (!entities_[index].isFilePath()) {
		return;
	}
	const auto check_sum = getFileChecksum(entities_[index].file_path, QCryptographicHash::Md5);
	ui_->audioMD5LineEdit->setText(check_sum);
}

void TagEditPage::closeEvent(QCloseEvent* event) {
	QFrame::closeEvent(event);
}

void TagEditPage::readEmbeddedCover(const PlayListEntity& entity) {
	TagIO tag_io;
	QSize image_size(0, 0);
	size_t image_file_size = 0;
	QPixmap image;
	tag_io.Open(entity.file_path.toStdWString(), TAG_IO_READ_MODE);
	if (tag_io.embeddedCover(image, image_file_size)) {
		image_size = image.size();
		image = image_util::resizeImage(image, ui_->coverLabel->size());
		ui_->notFoundImageLabel->hide();
		ui_->coverLabel->show();
	}
	else {
		ui_->notFoundImageLabel->show();
		ui_->coverLabel->hide();
		ui_->coverSizeLabel->hide();
	}

	setImageLabel(image, image_size, image_file_size);
	adjustSize();
}

void TagEditPage::setImageLabel(const QPixmap& image, QSize image_size, size_t image_file_size) {	
	ui_->coverLabel->setPixmap(image);
	ui_->coverSizeLabel->setText(stringFormat("{} x {} ({})",
		image_size.width(),
		image_size.height(),
		String::FormatBytes(image_file_size)));
}
