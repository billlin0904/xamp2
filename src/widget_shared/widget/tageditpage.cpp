#include <widget/databasefacade.h>
#include <widget/str_utilts.h>
#include <ui_tageditpage.h>
#include <widget/image_utiltis.h>
#include <widget/ui_utilts.h>
#include <widget/tageditpage.h>
#include <widget/xmessagebox.h>
#include <widget/imagecache.h>

TagEditPage::TagEditPage(QWidget* parent, const QList<PlayListEntity>& entities)
	: QFrame(parent)
	, entities_(entities) {
	ui_ = new Ui::TagEditPage();
	ui_->setupUi(this);

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
		if (!tag_io.CanWriteEmbeddedCover(entity.file_path.toStdWString())) {
			continue;
		}
		ui_->titleComboBox->addItem(entity.title);
	}

	Q_FOREACH(const auto entity, entities_) {
		if (!tag_io.CanWriteEmbeddedCover(entity.file_path.toStdWString())) {
			continue;
		}
		ui_->trackComboBox->addItem(QString::number(entity.track));
	}

	(void)QObject::connect(ui_->buttonBox, &QDialogButtonBox::accepted, [this]() {
		if (XMessageBox::ShowYesOrNo(qApp->tr("Do you want write tag?")) != QDialogButtonBox::Yes) {
			return;
		}

		const auto index = ui_->titleComboBox->currentIndex();
		const Path path = entities_[index].file_path.toStdWString();

		TagIO tag_io;
		tag_io.WriteArtist(path, ui_->artistLineEdit->text());
		tag_io.WriteTitle(path, ui_->titleComboBox->currentText());
		tag_io.WriteAlbum(path, ui_->albumLineEdit->text());
		tag_io.WriteComment(path, ui_->commentLineEdit->text());
		tag_io.WriteGenre(path, ui_->genreComboBox->currentText());
		tag_io.WriteTrack(path, ui_->trackComboBox->currentText().toUInt());
		tag_io.WriteYear(path, ui_->yearLineEdit->text().toUInt());
	});

	(void)QObject::connect(ui_->titleComboBox, &QComboBox::activated, [this](auto index) {
		ui_->artistLineEdit->setText(entities_[index].artist);
		ui_->albumLineEdit->setText(entities_[index].album);
		ui_->commentLineEdit->setText(entities_[index].comment);
		ui_->trackComboBox->setCurrentIndex(index);
		ReadEmbeddedCover(entities_[index]);
		});

	(void)QObject::connect(ui_->trackComboBox, &QComboBox::activated, [this](auto index) {
		ui_->artistLineEdit->setText(entities_[index].artist);
		ui_->albumLineEdit->setText(entities_[index].album);
		ui_->commentLineEdit->setText(entities_[index].comment);
		ui_->titleComboBox->setCurrentIndex(index);
		ReadEmbeddedCover(entities_[index]);
		});

	(void)QObject::connect(ui_->removeCoverButton, &QPushButton::clicked, [this] {
		});

	(void)QObject::connect(ui_->saveToFileButton, &QPushButton::clicked, [this] {
		if (XMessageBox::ShowYesOrNo(qApp->tr("Do you want write cover image?")) != QDialogButtonBox::Yes) {
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
		tag_io.WriteEmbeddedCover(entities_[index].file_path.toStdWString(), temp_image_);
		});

	(void)QObject::connect(ui_->addImageFileButton, &QPushButton::clicked, [=] {
		const auto index = ui_->titleComboBox->currentIndex();
		const auto dir = entities_[index].parent_path;
		constexpr auto file_ext = qTEXT("(*.jpg *.png *.bmp *.jpe *.jpeg *.tif *.tiff)");

		const auto action = [this, index](auto file_path) {
			const auto image = image_utils::ReadFileImage(file_path,
				kCoverSize,
				ImageCache::kFormat);
			if (image.isNull()) {
				return;
			}

			auto temp_image = image_utils::ResizeImage(image_utils::ConvertToImageFormat(image), kCoverSize);
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
			const auto resize_image = image_utils::ResizeImage(temp_image_, ui_->coverLabel->size());

			SetImageLabel(resize_image, image_size, image_file_size);
			};

		GetOpenFileName(parent,
			action,
			QWidget::tr("Open image file"),
			dir,
			QWidget::tr("Image Files ") + file_ext);
		});

	ui_->trackComboBox->setCurrentIndex(0);
	ui_->titleComboBox->setCurrentIndex(0);

	ui_->artistLineEdit->setText(entities_[0].artist);
	ui_->albumLineEdit->setText(entities_[0].album);
	ui_->commentLineEdit->setText(entities_[0].comment);

	ui_->coverSizeLabel->setText(StringFormat("{} x {} (0 B)", 0, 0));

	ui_->yearLineEdit->setValidator(new QIntValidator(1, 999));
	ReadEmbeddedCover(entities_[0]);
}

TagEditPage::~TagEditPage() {
	delete ui_;
}

void TagEditPage::ReadEmbeddedCover(const PlayListEntity& entity) {
	const TagIO tag_io;
	QSize image_size(0, 0);
	size_t image_file_size = 0;
	QPixmap image;

	if (tag_io.GetEmbeddedCover(entity.file_path.toStdWString(), image, image_file_size)) {
		image_size = image.size();
		image = image_utils::ResizeImage(image, ui_->coverLabel->size());		
	}

	SetImageLabel(image, image_size, image_file_size);
}

void TagEditPage::SetImageLabel(const QPixmap& image, QSize image_size, size_t image_file_size) {	
	ui_->coverLabel->setPixmap(image);
	ui_->coverSizeLabel->setText(StringFormat("{} x {} ({})",
		image_size.width(),
		image_size.height(),
		String::FormatBytes(image_file_size)));
}