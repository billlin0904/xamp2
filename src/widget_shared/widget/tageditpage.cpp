#include <widget/databasefacade.h>
#include <widget/str_utilts.h>
#include <widget/image_utiltis.h>
#include <widget/ui_utilts.h>
#include <widget/tageditpage.h>
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

		auto action = [this](auto file_path) {
			const auto image = image_utils::ReadFileImage(file_path,
				kCoverSize,
				ImageCache::kFormat);
			if (image.isNull()) {
				return;
			}
			const auto image_size = image.size();
			ui_.coverLabel->setPixmap(image_utils::ResizeImage(image, ui_.coverLabel->size()));
			ui_.coverSizeLabel->setText(StringFormat("{} x {}", image_size.width(), image_size.height()));
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

	ui_.coverSizeLabel->setText(StringFormat("{} x {}", 0, 0));

	ReadEmbeddedCover(entities_[0]);
}

void TagEditPage::ReadEmbeddedCover(const PlayListEntity& entity) {
	CoverArtReader cover_reader;
	const QPixmap image = cover_reader.GetEmbeddedCover(entity.file_path.toStdWString());
	const auto image_size = image.size();
	if (!image.isNull()) {
		ui_.coverLabel->setPixmap(image_utils::ResizeImage(image, ui_.coverLabel->size()));
	}
	else {
		ui_.coverLabel->setPixmap(QPixmap());
	}
	ui_.coverSizeLabel->setText(StringFormat("{} x {}", image_size.width(), image_size.height()));
}