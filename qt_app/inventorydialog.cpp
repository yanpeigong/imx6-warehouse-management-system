#include "inventorydialog.h"
#include "dbmanager.h"

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>

InventoryDialog::InventoryDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("库存查询");
    resize(620, 400);

    QLabel *title = new QLabel("📋 库存查询");
    QFont f = title->font();
    f.setPointSize(14); f.setBold(true);
    title->setFont(f);
    title->setStyleSheet("color:#1976D2; padding:6px;");

    editKeyword_ = new QLineEdit;
    editKeyword_->setPlaceholderText("按条码或名称搜索...");
    QPushButton *btnSearch = new QPushButton("搜索");
    QPushButton *btnRefresh= new QPushButton("刷新全部");
    QHBoxLayout *hb = new QHBoxLayout;
    hb->addWidget(editKeyword_); hb->addWidget(btnSearch); hb->addWidget(btnRefresh);

    table_ = new QTableWidget;
    table_->setColumnCount(6);
    table_->setHorizontalHeaderLabels(QStringList()
        << "ID" << "条码" << "名称" << "规格" << "单价(元)" << "库存");
    table_->horizontalHeader()->setStretchLastSection(false);
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);

    QPushButton *btnClose = new QPushButton("关闭");
    QHBoxLayout *hb2 = new QHBoxLayout;
    hb2->addStretch(); hb2->addWidget(btnClose);

    QVBoxLayout *vb = new QVBoxLayout(this);
    vb->addWidget(title);
    vb->addLayout(hb);
    vb->addWidget(table_);
    vb->addLayout(hb2);

    connect(btnSearch,  &QPushButton::clicked, this, &InventoryDialog::refresh);
    connect(btnRefresh, &QPushButton::clicked, [this]{
        editKeyword_->clear(); refresh();
    });
    connect(btnClose,   &QPushButton::clicked, this, &QDialog::accept);
    connect(editKeyword_, &QLineEdit::returnPressed, this, &InventoryDialog::refresh);

    refresh();
}

void InventoryDialog::refresh() {
    auto list = DbManager::instance().listAllProducts(editKeyword_->text().trimmed());
    table_->setRowCount(list.size());
    for (int i = 0; i < list.size(); ++i) {
        const auto &m = list[i];
        table_->setItem(i, 0, new QTableWidgetItem(QString::number(m["id"].toInt())));
        table_->setItem(i, 1, new QTableWidgetItem(m["barcode"].toString()));
        table_->setItem(i, 2, new QTableWidgetItem(m["name"].toString()));
        table_->setItem(i, 3, new QTableWidgetItem(m["spec"].toString()));
        table_->setItem(i, 4, new QTableWidgetItem(QString::number(m["price"].toDouble(), 'f', 2)));
        QTableWidgetItem *item = new QTableWidgetItem(QString::number(m["stock"].toInt()));
        if (m["stock"].toInt() <= 10) {
            item->setForeground(Qt::red);                     // 库存预警高亮
        }
        table_->setItem(i, 5, item);
    }
}
