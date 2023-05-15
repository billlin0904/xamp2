#include <widget/taglistview.h>

#include <widget/str_utilts.h>
#include <widget/ui_utilts.h>
#include <thememanager.h>

#include <QHBoxLayout>
#include <QLabel>

TagWidgetItem::TagWidgetItem(const QString& tag, QColor color, QListWidget* parent)
	: QListWidgetItem(parent)
	, color_(color)
	, tag_(tag) {
	setFlags(Qt::NoItemFlags);
}

QString TagWidgetItem::GetTag() const {
	return tag_;
}

bool TagWidgetItem::IsEnable() const {
	return enabled_;
}

void TagWidgetItem::SetEnable(bool enable) {
	enabled_ = enable;
	if (enabled_) {
		listWidget()->itemWidget(this)->setStyleSheet(
			qSTR("border-radius: 18px; background-color: %1;").arg(color_.name())
		);
	}
	else {
		QColor color = Qt::black;
		listWidget()->itemWidget(this)->setStyleSheet(
			qSTR("border-radius: 18px; background-color: %1;").arg(color.name())
		);
	}
	listWidget()->update();
}

void TagWidgetItem::Enable() {
	SetEnable(!IsEnable());
}

TagListView::TagListView(QWidget* parent)
	: QFrame(parent) {
	taglist_ = new QListWidget();
	taglist_->setDragEnabled(false);
	taglist_->setUniformItemSizes(true);
	taglist_->setFlow(QListView::LeftToRight);
	taglist_->setResizeMode(QListView::Adjust);
	taglist_->setFrameStyle(QFrame::StyledPanel);
	taglist_->setViewMode(QListView::IconMode);
	taglist_->setFixedHeight(100);

	taglist_->setStyleSheet(qTEXT(
		"QListWidget {"
		"  border: none;"
		"} "
		"QListWidget::item {"
		"  border: 1px solid transparent;"
		"  border-radius: 18px;"
		"  background-color: transparent;"
		"}"
		"QListWidget::item:selected {"
		"  background-color: transparent;"
		"}"
	));

	(void)QObject::connect(taglist_, &QListWidget::itemClicked, [this](auto* item) {
		if (!item) {
			return;
		}

		auto* tag_item = dynamic_cast<TagWidgetItem*>(item);
		if (!tag_item) {
			return;
		}
		tag_item->Enable();
		if (!tag_item->IsEnable()) {
			tags_.remove(tag_item->GetTag());
		}
		else {
			tags_.insert(tag_item->GetTag());
		}
		if (!tags_.isEmpty()) {
			emit TagChanged(tags_);
		}
		else {
			emit TagClear();
		}
		});

	auto* tagLayout = new QVBoxLayout();
	tagLayout->addWidget(taglist_);
	setLayout(tagLayout);
}

void TagListView::AddTag(const QString& tag) {
	auto color = GenerateRandomColor();

	auto* item = new TagWidgetItem(tag, color, taglist_);

	auto f = font();
	f.setBold(true);
	f.setPointSize(qTheme.GetFontSize(10));
	item->setSizeHint(QSize(170, 40));

	auto* layout = new QHBoxLayout();
	auto* tag_label = new QLabel(tag);
	tag_label->setFont(f);
	tag_label->setAlignment(Qt::AlignCenter);

	layout->addWidget(tag_label);
	layout->setSpacing(0);
	layout->setContentsMargins(0, 0, 0, 0);

	auto* widget = new QWidget();
	widget->setLayout(layout);
	widget->setStyleSheet(
		qSTR("border-radius: 18px; background-color: %1;").arg(color.name())
	);
	taglist_->setItemWidget(item, widget);
	item->Enable();	
}

void TagListView::ClearTag() {
	for (auto i = 0; i < taglist_->count(); ++i) {
		auto item = (TagWidgetItem*)taglist_->item(i);
		item->SetEnable(false);
	}
}