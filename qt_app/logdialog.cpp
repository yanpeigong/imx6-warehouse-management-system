#include "logdialog.h"
#include "dbmanager.h"

#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>

LogDialog::LogDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("操作日志");
    resize(800, 500);

    QLabel *title = new QLabel("📜 操作日志（最近100条）");
    QFont f = title->font();
    f.setPointSize(14); f.setBold(true);
    title->setFont(f);
    title->setStyleSheet("color:#7B1FA2; padding:6px;");

    table_ = new QTableWidget;
    table_->setColumnCount(7);
    table_->setHorizontalHeaderLabels(QStringList()
        << "ID" << "类型" << "条码" << "商品名称" << "数量" << "操作员" << "时间");
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);

    QPushButton *btnRefresh = new QPushButton("刷新");
    QPushButton *btnClose   = new QPushButton("关闭");
    QHBoxLayout *hb = new QHBoxLayout;
    hb->addWidget(btnRefresh); hb->addStretch(); hb->addWidget(btnClose);

    QVBoxLayout *vb = new QVBoxLayout(this);
    vb->addWidget(title);
    vb->addWidget(table_);
    vb->addLayout(hb);

    connect(btnRefresh, &QPushButton::clicked, this, &LogDialog::refresh);
    connect(btnClose,   &QPushButton::clicked, this, &QDialog::accept);

    refresh();
}

void LogDialog::refresh() {
    auto list = DbManager::instance().recentOperations(100);
    table_->setRowCount(list.size());
    for (int i = 0; i < list.size(); ++i) {
        const auto &m = list[i];
        table_->setItem(i, 0, new QTableWidgetItem(QString::number(m["id"].toInt())));
        QString op = m["op_type"].toString();
        QTableWidgetItem *opItem = new QTableWidgetItem(op == "in" ? "入库" : "出库");
        opItem->setForeground(op == "in" ? QColor("#388E3C") : QColor("#D32F2F"));
        table_->setItem(i, 1, opItem);
        table_->setItem(i, 2, new QTableWidgetItem(m["barcode"].toString()));
        table_->setItem(i, 3, new QTableWidgetItem(m["name"].toString()));
        table_->setItem(i, 4, new QTableWidgetItem(QString::number(m["quantity"].toInt())));
        table_->setItem(i, 5, new QTableWidgetItem(m["operator"].toString()));
        table_->setItem(i, 6, new QTableWidgetItem(m["op_time"].toString()));
    }
}
