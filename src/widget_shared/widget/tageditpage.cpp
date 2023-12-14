#include <QCloseEvent>
#include <QCryptographicHash>

#include <ui_tageditpage.h>

#include <widget/databasefacade.h>
#include <widget/str_utilts.h>
#include <widget/database.h>
#include <widget/image_utiltis.h>
#include <widget/ui_utilts.h>
#include <widget/tageditpage.h>
#include <widget/xmessagebox.h>
#include <widget/tagio.h>
#include <widget/imagecache.h>

namespace {
	QString GetFileChecksum(const QString& file_name, 
		const QCryptographicHash::Algorithm hash_algorithm) {
		QFile f(file_name);
		if (f.open(QFile::ReadOnly)) {
			QCryptographicHash hash(hash_algorithm);
			if (hash.addData(&f)) {
				return hash.result().toHex().toUpper();
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
	};

	auto font = qTheme.GetFormatFont();
	font.setPointSize(qTheme.GetFontSize(9));
	ui_->coverSizeLabel->setFont(font);
	ui_->albumPeakLineEdit->setFont(font);
	ui_->albumReplayGainLineEdit->setFont(font);
	ui_->trackPeakLineEdit->setFont(font);
	ui_->trackReplayGainLineEdit->setFont(font);
	ui_->audioMD5LineEdit->setFont(font);
	ui_->fileSizeLineEdit->setFont(font);

	for (auto& l : labels) {
		l->setStyleSheet(qTEXT("background-color: transparent;"));
	}

	const TagIO tag_io;

	const QStringList genre_list{
		"",
		"Blues",
		"Classic Rock",
		"Country",
		"Dance",
		"Disco",
		"Funk",
		"Grunge",
		"Hip-Hop",
		"Jazz",
		"Metal",
		"New Age",
		"Oldies",
		"Other",
		"Pop",
		"R&B",
		"Rap",
		"Reggae",
		"Rock",
		"Techno",
		"Industrial",
		"Alternative",
		"Ska",
		"Death Metal",
		"Pranks",
		"Soundtrack",
		"Euro-Techno",
		"Ambient",
		"Trip-Hop",
		"Vocal",
	};

	ui_->genreComboBox->addItems(genre_list);

	std::ranges::sort(entities_, [](auto a, auto b) {
		return a.track < b.track;
		});

	Q_FOREACH (const auto entity, entities_) {
		ui_->titleComboBox->addItem(entity.title);
	}

	Q_FOREACH(const auto entity, entities_) {
		ui_->trackComboBox->addItem(QString::number(entity.track));
	}

	(void)QObject::connect(ui_->buttonBox, &QDialogButtonBox::accepted, [this]() {
		if (XMessageBox::showYesOrNo(qApp->tr("Do you want write tag?")) != QDialogButtonBox::Yes) {
			return;
		}

		auto index = ui_->titleComboBox->currentIndex();

		auto& entity = entities_[index];
		const Path path = entity.file_path.toStdWString();

		try {
			TagIO tag_io;
			tag_io.WriteArtist(path, ui_->artistLineEdit->text());
			tag_io.WriteTitle(path, ui_->titleComboBox->currentText());
			tag_io.WriteAlbum(path, ui_->albumLineEdit->text());
			tag_io.WriteComment(path, ui_->commentLineEdit->text());
			tag_io.WriteGenre(path, ui_->genreComboBox->currentText());
			tag_io.WriteTrack(path, ui_->trackComboBox->currentText().toUInt());
			tag_io.WriteYear(path, ui_->yearLineEdit->text().toUInt());

			entity.artist = ui_->artistLineEdit->text();
			entity.title = ui_->titleComboBox->currentText();
			entity.album = ui_->albumLineEdit->text();
			entity.comment = ui_->commentLineEdit->text();
			entity.track = ui_->trackComboBox->currentText().toUInt();
			entity.year = ui_->yearLineEdit->text().toUInt();

			qMainDb.updateAlbum(entity.album_id, entity.album);
			qMainDb.updateArtist(entity.artist_id, entity.artist);
		} catch (...) {
			XMessageBox::showError(qApp->tr("Write tag failure!"));
			return;
		}		

		auto next_index = (index + 1) % entities_.size();
		if (next_index == entities_.size() - 1) {
			return;
		}

		ui_->titleComboBox->setCurrentIndex(next_index);
		ui_->trackComboBox->setCurrentIndex(next_index);
		SetCurrentInfo(next_index);

		XMessageBox::showInformation(qApp->tr("Write tag successfully!"));
	});

	(void)QObject::connect(ui_->titleComboBox, &QComboBox::activated, [this](auto index) {
		SetCurrentInfo(index);
		ReadEmbeddedCover(entities_[index]);
		});

	(void)QObject::connect(ui_->trackComboBox, &QComboBox::activated, [this](auto index) {
		SetCurrentInfo(index);
		ReadEmbeddedCover(entities_[index]);
		});

	(void)QObject::connect(ui_->clearCommentButton, &QPushButton::clicked, [this] {
		ui_->commentLineEdit->setText("");
		});

	(void)QObject::connect(ui_->removeCoverButton, &QPushButton::clicked, [this] {
		TagIO tag_io;
		const auto index = ui_->titleComboBox->currentIndex();
		tag_io.RemoveEmbeddedCover(entities_[index].file_path.toStdWString());
		});

	(void)QObject::connect(ui_->saveToFileButton, &QPushButton::clicked, [this] {
		if (XMessageBox::showYesOrNo(qApp->tr("Do you want write cover image?")) != QDialogButtonBox::Yes) {
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

		if (!tag_io.CanWriteEmbeddedCover(entities_[index].file_path.toStdWString())) {
			return;
		}

		tag_io.WriteEmbeddedCover(entities_[index].file_path.toStdWString(), temp_image_);
		});

	(void)QObject::connect(ui_->addImageFileButton, &QPushButton::clicked, [=] {
		const auto index = ui_->titleComboBox->currentIndex();
		const auto dir = entities_[index].parent_path;
		
		const auto action = [this, index](auto file_path) {
			const auto image = image_utils::readFileImage(file_path,
				kCoverSize,
				ImageCache::kFormat);
			if (image.isNull()) {
				return;
			}

			//auto temp_image = image_utils::ResizeImage(image_utils::ConvertToImageFormat(image), kCoverSize);
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
			const auto resize_image = image_utils::resizeImage(temp_image_, ui_->coverLabel->size());

			SetImageLabel(resize_image, image_size, image_file_size);
			};

		GetOpenFileName(parent,
			action,
			QWidget::tr("Open image file"),
			dir,
			QWidget::tr("Image Files ") + kCoverImageFileExt);
		});

	ui_->notFoundImageLabel->hide();
	ui_->trackComboBox->setCurrentIndex(0);
	ui_->titleComboBox->setCurrentIndex(0);

	SetCurrentInfo(0);

	ui_->coverSizeLabel->setText(StringFormat("{} x {} (0 B)", 0, 0));
	ui_->yearLineEdit->setValidator(new QIntValidator(1, 9999));

	ReadEmbeddedCover(entities_[0]);
}

TagEditPage::~TagEditPage() {
	delete ui_;
}

void TagEditPage::SetCurrentInfo(int32_t index) {
	ui_->artistLineEdit->setText(entities_[index].artist);
	ui_->albumLineEdit->setText(entities_[index].album);
	ui_->commentLineEdit->setText(entities_[index].comment);
	ui_->yearLineEdit->setText(QString::number(entities_[index].year));
	ui_->albumPeakLineEdit->setText(FormatDb(entities_[index].album_peak));
	ui_->albumReplayGainLineEdit->setText(FormatDb(entities_[index].album_replay_gain));
	ui_->trackPeakLineEdit->setText(FormatDb(entities_[index].track_peak));
	ui_->trackReplayGainLineEdit->setText(FormatDb(entities_[index].track_replay_gain));
	ui_->fileSizeLineEdit->setText(StringFormat("{} ({})",
		entities_[index].file_size,
		String::FormatBytes(entities_[index].file_size)));
	ui_->titleComboBox->setCurrentIndex(index);
	ui_->trackComboBox->setCurrentIndex(index);

	const auto check_sum = GetFileChecksum(entities_[index].file_path, QCryptographicHash::Md5);
	ui_->audioMD5LineEdit->setText(check_sum);
}

void TagEditPage::closeEvent(QCloseEvent* event) {
	/*if (XMessageBox::ShowYesOrNo(tr("Do you give up write tag ?")) == QDialogButtonBox::No) {
		event->ignore();
		return;
	}*/
	QFrame::closeEvent(event);
}

void TagEditPage::ReadEmbeddedCover(const PlayListEntity& entity) {
	const TagIO tag_io;
	QSize image_size(0, 0);
	size_t image_file_size = 0;
	QPixmap image;

	if (tag_io.GetEmbeddedCover(entity.file_path.toStdWString(), image, image_file_size)) {
		image_size = image.size();
		image = image_utils::resizeImage(image, ui_->coverLabel->size());
		ui_->notFoundImageLabel->hide();
		ui_->coverLabel->show();
	}
	else {
		ui_->notFoundImageLabel->show();
		ui_->coverLabel->hide();
	}

	SetImageLabel(image, image_size, image_file_size);
	adjustSize();
}

void TagEditPage::SetImageLabel(const QPixmap& image, QSize image_size, size_t image_file_size) {	
	ui_->coverLabel->setPixmap(image);
	ui_->coverSizeLabel->setText(StringFormat("{} x {} ({})",
		image_size.width(),
		image_size.height(),
		String::FormatBytes(image_file_size)));
}