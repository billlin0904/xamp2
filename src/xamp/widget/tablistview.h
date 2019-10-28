#pragma once

#include <QListView>
#include <QStandardItemModel>

class TabListView : public QListView {
    Q_OBJECT
public:
    explicit TabListView(QWidget *parent = nullptr);

    void addTab(const QString& name, int table_id);

signals:
    void clickedTable(int table_id);

private:
    QStandardItemModel model_;
};

