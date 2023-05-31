#include <widget/taglistview.h>

#include <widget/str_utilts.h>
#include <widget/ui_utilts.h>
#include <thememanager.h>

#include <QHBoxLayout>
#include <QLabel>

TagWidgetItem::TagWidgetItem(const QString& tag, QColor color, QLabel* label, QListWidget* parent)
	: QListWidgetItem(tag, parent)
	, color_(color)
	, tag_(tag)
	, label_(label) {
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
		switch (qTheme.GetThemeColor()) {
		case ThemeColor::DARK_THEME:
			listWidget()->itemWidget(this)->setStyleSheet(
				qSTR("border-radius: 6px; border: 1px solid %1; background-color: #171818;").arg(color_.name())
			);
			break;
		case ThemeColor::LIGHT_THEME:
			listWidget()->itemWidget(this)->setStyleSheet(
				qSTR("border-radius: 6px; border: 1px solid %1; background-color: #e6e6e6;").arg(color_.name())
			);
		}		
		label_->setStyleSheet(
			qSTR("color: %1;").arg(color_.name())
		);
	}
	else {
		QColor color;
		switch (qTheme.GetThemeColor()) {
		case ThemeColor::DARK_THEME:
			color = QColor(qTEXT("#2e2f31"));
			listWidget()->itemWidget(this)->setStyleSheet(
				qSTR("border-radius: 6px; border: 1px solid %1; background-color: #2f3233;").arg(color.name())
			);
			label_->setStyleSheet(
				qSTR("color: white;")
			);
			break;
		case ThemeColor::LIGHT_THEME:
			color = Qt::lightGray;
			listWidget()->itemWidget(this)->setStyleSheet(
				qSTR("border-radius: 6px; border: 1px solid transparent; background-color: #e6e6e6;")
			);
			label_->setStyleSheet(
				qSTR("color: #2e2f31;")
			);
			break;
		}				
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
	taglist_->setUniformItemSizes(false);
	taglist_->setFlow(QListView::LeftToRight);
	taglist_->setResizeMode(QListView::Adjust);
	taglist_->setFrameStyle(QFrame::StyledPanel);
	taglist_->setViewMode(QListView::IconMode);
	taglist_->setFixedHeight(100);
	taglist_->setSpacing(5);	

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

void TagListView::SetListViewFixedHeight(int32_t height) {
	taglist_->setFixedHeight(height);
}

void TagListView::OnCurrentThemeChanged(ThemeColor theme_color) {
	switch (theme_color) {
	case ThemeColor::DARK_THEME:
		taglist_->setStyleSheet(qTEXT(
		"QListWidget {"
		"  border: none;"
		"}"
		"QListWidget::item {"
		"  border: 1px solid transparent;"
		"  border-radius: 8px;"
		"  background-color: #2f3233;"
		"}"
		"QListWidget::item:selected {"
		"  background-color: transparent;"
		"}"
		));	
		break;
	case ThemeColor::LIGHT_THEME:
		taglist_->setStyleSheet(qTEXT(
			"QListWidget {"
			"  border: none;"
			"}"
			"QListWidget::item {"
			"  border: 1px solid transparent;"
			"  border-radius: 8px;"
			"  background-color: #e6e6e6;"
			"}"
			"QListWidget::item:selected {"
			"  background-color: transparent;"
			"}"
		));
		break;
	}
}

void TagListView::OnThemeColorChanged(QColor backgroundColor, QColor color) {

}

void TagListView::AddTag(const QString& tag, bool uniform_item_sizes) {
	auto items = taglist_->findItems(tag, Qt::MatchContains);
	if (!items.isEmpty()) {
		return;
	}
	
	auto color = qTheme.GetHighlightColor();

	auto f = font();
	auto* layout = new QHBoxLayout();
	auto* tag_label = new QLabel(tag);	
	tag_label->setAlignment(Qt::AlignCenter);	

	auto* item = new TagWidgetItem(tag, color, tag_label, taglist_);
	f.setBold(true);
	f.setPointSize(qTheme.GetFontSize(12));
	tag_label->setFont(f);

	if (!uniform_item_sizes) {
		QFontMetrics metrics(f);
		auto width = metrics.horizontalAdvance(tag) * 1.3;
		item->setSizeHint(QSize(width, 40));
	}
	else {
		item->setSizeHint(QSize(80, 40));
	}

	layout->addWidget(tag_label);
	layout->setSpacing(0);
	layout->setContentsMargins(0, 0, 0, 0);

	auto* widget = new QWidget();
	widget->setLayout(layout);
	widget->setStyleSheet(
		qSTR("border-radius: 6px; border: 1px solid transparent;")
	);
	taglist_->setItemWidget(item, widget);
	item->Enable();	
}

void TagListView::ClearTag() {
	for (auto i = 0; i < taglist_->count(); ++i) {
		auto item = dynamic_cast<TagWidgetItem*>(taglist_->item(i));
		if (!item) {
			continue;
		}
		item->SetEnable(false);
	}
}