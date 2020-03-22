#include <QtGui>

#include <widget/stareditor.h>
#include <widget/starrating.h>
#include <widget/stardelegate.h>

void StarDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    if (index.data().canConvert<StarRating>()) {
        auto rating = qvariant_cast<StarRating>(index.data());

        if (option.state & QStyle::State_Selected)
            painter->fillRect(option.rect, Qt::darkGray);

        rating.paint(painter, option.rect, option.palette, StarRating::ReadOnly);
    }
    else {
        QStyledItemDelegate::paint(painter, option, index);
    }
}

QSize StarDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    if (index.data().canConvert<StarRating>()) {
        auto rating = qvariant_cast<StarRating>(index.data());
        return rating.sizeHint();
    }
    else {
        return QStyledItemDelegate::sizeHint(option, index);
    }
}

QWidget* StarDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    if (index.data().canConvert<StarRating>()) {
        auto editor = new StarEditor(index.row(), parent);
        (void)QObject::connect(editor, &StarEditor::editingFinished, this, &StarDelegate::commitAndCloseEditor);
        return editor;
    }
    else {
        return QStyledItemDelegate::createEditor(parent, option, index);
    }
}

void StarDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const {
    if (index.data().canConvert<StarRating>()) {
        auto rating = qvariant_cast<StarRating>(index.data());
        auto star_editor = qobject_cast<StarEditor*>(editor);
        star_editor->setStarRating(rating);
    }
    else {
        QStyledItemDelegate::setEditorData(editor, index);
    }
}

void StarDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const {
    if (index.data().canConvert<StarRating>()) {
        auto star_editor = qobject_cast<StarEditor*>(editor);
        model->setData(index, QVariant::fromValue(star_editor->starRating()));
    }
    else {
        QStyledItemDelegate::setModelData(editor, model, index);
    }
}

void StarDelegate::commitAndCloseEditor() {
    auto editor = qobject_cast<StarEditor*>(sender());
    emit commitData(editor);
    emit closeEditor(editor);
}