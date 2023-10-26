#include <widget/databasefacade.h>
#include <widget/str_utilts.h>
#include <widget/image_utiltis.h>
#include <widget/ui_utilts.h>
#include <widget/tageditpage.h>
#include <widget/xmessagebox.h>
#include <widget/imagecache.h>

TagEditPage::TagEditPage(QWidget* parent, const QList<PlayListEntity>& entities)
	: QFrame(parent)
	, entities_(entities) {
	ui_.setupUi(this);
	
	Q_FOREACH (const auto entity, entities_) {
		ui_.titleComboBox->addItem(entity.title);
	}

	Q_FOREACH(const auto entity, entities_) {
		ui_.trackComboBox->addItem(QString::number(entity.track));
	}	

	(void)QObject::connect(ui_.titleComboBox, &QComboBox::activated, [this](auto index) {
		ui_.artistLineEdit->setText(entities_[index].artist);
		ui_.albumLineEdit->setText(entities_[index].album);
		ui_.commentLineEdit->setText(entities_[index].comment);
		ui_.trackComboBox->setCurrentIndex(index);
		ReadEmbeddedCover(entities_[index]);
		});

	(void)QObject::connect(ui_.trackComboBox, &QComboBox::activated, [this](auto index) {
		ui_.artistLineEdit->setText(entities_[index].artist);
		ui_.albumLineEdit->setText(entities_[index].album);
		ui_.commentLineEdit->setText(entities_[index].comment);
		ui_.titleComboBox->setCurrentIndex(index);
		ReadEmbeddedCover(entities_[index]);
		});

	(void)QObject::connect(ui_.addImageFileButton, &QPushButton::clicked, [=] {
		const auto index = ui_.titleComboBox->currentIndex();
		const auto dir = entities_[index].parent_path;
		constexpr auto file_ext = qTEXT("(*.jpg *.png *.bmp *.jpe *.jpeg *.tif *.tiff)");

		const auto action = [this, index](auto file_path) {
			const auto image = image_utils::ReadFileImage(file_path,
				kCoverSize,
				ImageCache::kFormat);
			if (image.isNull()) {
				return;
			}

			if (XMessageBox::ShowYesOrNo(qApp->tr("Do you want write cover image?")) != QDialogButtonBox::Ok) {
				return;
			}

			auto jpeg_image = image_utils::ResizeImage(image_utils::ConvertToImageFormat(image), kCoverSize);
			auto std_temp_file_path = GetTempFileNamePath();
			auto temp_file_path = QString::fromStdWString(std_temp_file_path.wstring());
			if (!jpeg_image.save(temp_file_path, "JPG")) {
				return;
			}
		
			auto image_file_size = QFileInfo(temp_file_path).size();
			const auto image_size = jpeg_image.size();
			auto resize_image = image_utils::ResizeImage(jpeg_image, ui_.coverLabel->size());

			SetImageLabel(resize_image, image_size, image_file_size);

			CoverArtReader cover_reader;
			cover_reader.WriteEmbeddedCover(entities_[index].file_path.toStdWString(), jpeg_image);

			try {
				Fs::remove(std_temp_file_path);
			} catch (...) {}
			};

		GetOpenFileName(parent,
			action,
			QWidget::tr("Open image file"),
			dir,
			QWidget::tr("Image Files ") + file_ext);
		});

	ui_.trackComboBox->setCurrentIndex(0);
	ui_.titleComboBox->setCurrentIndex(0);

	ui_.artistLineEdit->setText(entities_[0].artist);
	ui_.albumLineEdit->setText(entities_[0].album);
	ui_.commentLineEdit->setText(entities_[0].comment);

	ui_.coverSizeLabel->setText(StringFormat("{} x {} (0 B)", 0, 0));

	ReadEmbeddedCover(entities_[0]);
}

void TagEditPage::ReadEmbeddedCover(const PlayListEntity& entity) {
	CoverArtReader cover_reader;
	QSize image_size(0, 0);
	size_t image_file_size = 0;
	QPixmap image;

	if (cover_reader.GetEmbeddedCover(entity.file_path.toStdWString(), image, image_file_size)) {
		image_size = image.size();
		image = image_utils::ResizeImage(image, ui_.coverLabel->size());		
	}

	SetImageLabel(image, image_size, image_file_size);
}

void TagEditPage::SetImageLabel(const QPixmap& image, QSize image_size, size_t image_file_size) {	
	ui_.coverLabel->setPixmap(image);
	ui_.coverSizeLabel->setText(StringFormat("{} x {} ({})",
		image_size.width(),
		image_size.height(),
		String::FormatBytes(image_file_size)));
}