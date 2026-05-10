#include "stockoutdialog.h"
#include "serialreader.h"
#include "dbmanager.h"

#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QInputDialog>
#include <QMessageBox>
#include <QApplication>

StockOutDialog::StockOutDialog(SerialReader *reader, const QString &operatorName, QWidget *parent)
    : QDialog(parent), reader_(reader), operatorName_(operatorName) {
    setWindowTitle("出库");
    resize(520, 480);

    QLabel *title = new QLabel("📤 出库 — 请扫描商品条码");
    QFont f = title->font();
    f.setPointSize(14); f.setBold(true);
    title->setFont(f);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("color:#D32F2F; padding:8px;");

    QGroupBox *grp = new QGroupBox("商品信息");
    lblBarcode_ = new QLabel("--");
    lblName_    = new QLabel("--");
    lblSpec_    = new QLabel("--");
    lblPrice_   = new QLabel("--");
    lblStock_   = new QLabel("--");
    QFormLayout *fl = new QFormLayout(grp);
    fl->addRow("条  码:", lblBarcode_);
    fl->addRow("名  称:", lblName_);
    fl->addRow("规  格:", lblSpec_);
    fl->addRow("单  价:", lblPrice_);
    fl->addRow("当前库存:", lblStock_);

    spinQty_ = new QSpinBox;
    spinQty_->setRange(1, 99999);
    spinQty_->setValue(1);
    editRemark_ = new QLineEdit;
    editRemark_->setPlaceholderText("可选");
    QFormLayout *fl2 = new QFormLayout;
    fl2->addRow("出库数量:", spinQty_);
    fl2->addRow("备  注:", editRemark_);

    btnOk_ = new QPushButton("确认出库");
    btnOk_->setEnabled(false);
    QPushButton *btnManual = new QPushButton("手动输入条码");
    QPushButton *btnClose  = new QPushButton("关闭");
    QHBoxLayout *hb = new QHBoxLayout;
    hb->addWidget(btnManual);
    hb->addStretch();
    hb->addWidget(btnOk_);
    hb->addWidget(btnClose);

    lblTip_ = new QLabel;

    QVBoxLayout *vb = new QVBoxLayout(this);
    vb->addWidget(title);
    vb->addWidget(grp);
    vb->addLayout(fl2);
    vb->addWidget(lblTip_);
    vb->addStretch();
    vb->addLayout(hb);

    if (reader_) {
        connect(reader_, &SerialReader::barcodeScanned,
                this,    &StockOutDialog::onBarcodeScanned);
    }
    connect(btnOk_,    &QPushButton::clicked, this, &StockOutDialog::onConfirmClicked);
    connect(btnManual, &QPushButton::clicked, this, &StockOutDialog::onManualInputClicked);
    connect(btnClose,  &QPushButton::clicked, this, &QDialog::accept);
}

void StockOutDialog::onBarcodeScanned(const QString &barcode) {
    if (!isActiveWindow()) return;
    loadProduct(barcode);
}

void StockOutDialog::onManualInputClicked() {
    bool ok = false;
    QString bc = QInputDialog::getText(this, "手动输入条码", "请输入条码:",
                                        QLineEdit::Normal, "", &ok);
    if (ok && !bc.trimmed().isEmpty()) {
        loadProduct(bc.trimmed());
    }
}

void StockOutDialog::loadProduct(const QString &barcode) {
    QVariantMap p = DbManager::instance().findProductByBarcode(barcode);
    if (p.isEmpty()) {
        QMessageBox::warning(this, "未找到",
            QString("条码 %1 不存在，请先入库该商品").arg(barcode));
        return;
    }
    currentProduct_ = p;
    lblBarcode_->setText(p["barcode"].toString());
    lblName_->setText(p["name"].toString());
    lblSpec_->setText(p["spec"].toString());
    lblPrice_->setText(QString::number(p["price"].toDouble(), 'f', 2) + " 元");
    int stock = p["stock"].toInt();
    lblStock_->setText(QString::number(stock));
    if (stock <= 0) {
        lblStock_->setStyleSheet("color:red; font-weight:bold;");
        lblTip_->setText("⚠ 当前库存为 0，无法出库");
        lblTip_->setStyleSheet("color:red;");
        btnOk_->setEnabled(false);
    } else {
        lblStock_->setStyleSheet("");
        spinQty_->setMaximum(stock);
        spinQty_->setValue(1);
        btnOk_->setEnabled(true);
        lblTip_->setText("已加载商品，请输入出库数量后点击 [确认出库]");
        lblTip_->setStyleSheet("color:#388E3C;");
        spinQty_->setFocus();
    }
    QApplication::beep();
}

void StockOutDialog::clearProduct() {
    currentProduct_.clear();
    lblBarcode_->setText("--");
    lblName_->setText("--");
    lblSpec_->setText("--");
    lblPrice_->setText("--");
    lblStock_->setText("--");
    lblStock_->setStyleSheet("");
    spinQty_->setValue(1);
    editRemark_->clear();
    btnOk_->setEnabled(false);
}

void StockOutDialog::onConfirmClicked() {
    if (currentProduct_.isEmpty()) return;
    int pid = currentProduct_["id"].toInt();
    QString bc = currentProduct_["barcode"].toString();
    int stock = currentProduct_["stock"].toInt();
    int qty = spinQty_->value();

    if (qty > stock) {
        QMessageBox::warning(this, "库存不足",
            QString("当前库存 %1，无法出库 %2").arg(stock).arg(qty));
        return;
    }

    DbManager &db = DbManager::instance();
    if (!db.updateStock(pid, -qty)) {
        QMessageBox::warning(this, "失败", "更新库存失败");
        return;
    }
    if (!db.insertOperation("out", pid, bc, qty, operatorName_, editRemark_->text())) {
        QMessageBox::warning(this, "警告", "流水记录失败");
    }
    lblTip_->setText(QString("✓ 出库成功：%1 × %2").arg(currentProduct_["name"].toString()).arg(qty));
    lblTip_->setStyleSheet("color:#388E3C;");
    clearProduct();
}
