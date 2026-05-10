#include "stockindialog.h"
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
 
StockInDialog::StockInDialog(SerialReader *reader, const QString &operatorName, QWidget *parent)
    : QDialog(parent), reader_(reader), operatorName_(operatorName) {
    setWindowTitle("入库");
    resize(460, 380);
 
    // ===== 顶部提示 =====
    QLabel *title = new QLabel("📦 入库 — 请扫描商品条码");
    QFont f = title->font();
    f.setPointSize(14); f.setBold(true);
    title->setFont(f);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("color:#1976D2; padding:8px;");
 
    // ===== 商品信息显示区 =====
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
 
    // ===== 输入区 =====
    spinQty_ = new QSpinBox;
    spinQty_->setRange(1, 99999);
    spinQty_->setValue(1);
 
    editRemark_ = new QLineEdit;
    editRemark_->setPlaceholderText("可选");
 
    QFormLayout *fl2 = new QFormLayout;
    fl2->addRow("入库数量:", spinQty_);
    fl2->addRow("备  注:", editRemark_);
 
    // ===== 按钮 =====
    btnOk_ = new QPushButton("确认入库");
    btnOk_->setEnabled(false);            // 没有商品时不可点
    QPushButton *btnManual = new QPushButton("手动输入条码");
    QPushButton *btnClose  = new QPushButton("关闭");
    QHBoxLayout *hb = new QHBoxLayout;
    hb->addWidget(btnManual);
    hb->addStretch();
    hb->addWidget(btnOk_);
    hb->addWidget(btnClose);
 
    lblTip_ = new QLabel;
    lblTip_->setStyleSheet("color:#388E3C; padding:4px;");
 
    QVBoxLayout *vb = new QVBoxLayout(this);
    vb->addWidget(title);
    vb->addWidget(grp);
    vb->addLayout(fl2);
    vb->addWidget(lblTip_);
    vb->addStretch();
    vb->addLayout(hb);
 
    // ===== 信号槽 =====
    if (reader_) {
        connect(reader_, &SerialReader::barcodeScanned,
                this,    &StockInDialog::onBarcodeScanned);
    }
    connect(btnOk_,    &QPushButton::clicked, this, &StockInDialog::onConfirmClicked);
    connect(btnManual, &QPushButton::clicked, this, &StockInDialog::onManualInputClicked);
    connect(btnClose,  &QPushButton::clicked, this, &QDialog::accept);
}
 
void StockInDialog::onBarcodeScanned(const QString &barcode) {
    // 只在本对话框是当前活动窗口时响应（避免出库/查询界面也被触发）
    if (!isActiveWindow()) return;
    loadProduct(barcode);
}
 
void StockInDialog::onManualInputClicked() {
    bool ok = false;
    QString bc = QInputDialog::getText(this, "手动输入条码", "请输入条码:",
                                        QLineEdit::Normal, "", &ok);
    if (ok && !bc.trimmed().isEmpty()) {
        loadProduct(bc.trimmed());
    }
}
 
void StockInDialog::loadProduct(const QString &barcode) {
    QVariantMap p = DbManager::instance().findProductByBarcode(barcode);
 
    if (p.isEmpty()) {
        // ===== 职责分离设计 =====
        // 未登记商品的录入由管理者在 Web 端完成，LCD 端只负责扫码出入库。
        // 这样设计的考虑：
        //   1. 业务合理性：商品上架是管理决策，应在管理后台进行
        //   2. 用户体验：LCD 触屏不适合输入中文（商品名称、规格通常含中文）
        //   3. 数据准确性：集中由管理者录入，避免现场员工录入错误
        clearProduct();
        lblBarcode_->setText(barcode);
        lblName_->setText("⚠ 未登记");
        lblName_->setStyleSheet("color:#D32F2F; font-weight:bold;");
        lblTip_->setText(
            QString("⚠ 条码 %1 未登记。\n请联系管理员在 Web 端【商品管理】中录入此商品。")
                .arg(barcode));
        lblTip_->setStyleSheet("color:#D32F2F; padding:4px; font-weight:bold;");
 
        QMessageBox::information(this, "未登记商品",
            QString("条码：%1\n\n该商品尚未登记到系统。\n\n请联系管理员在 Web 端的【商品管理】"
                    "页面中录入此商品后，再进行入库操作。").arg(barcode));
        return;
    }
 
    currentProduct_ = p;
    lblBarcode_->setText(p["barcode"].toString());
    lblName_->setText(p["name"].toString());
    lblName_->setStyleSheet("");                   // 恢复正常颜色
    lblSpec_->setText(p["spec"].toString());
    lblPrice_->setText(QString::number(p["price"].toDouble(), 'f', 2) + " 元");
    lblStock_->setText(QString::number(p["stock"].toInt()));
    btnOk_->setEnabled(true);
    spinQty_->setFocus();
    spinQty_->selectAll();
    lblTip_->setText("已加载商品，请输入入库数量后点击 [确认入库]");
    lblTip_->setStyleSheet("color:#388E3C; padding:4px;");
    QApplication::beep();
}
 
void StockInDialog::clearProduct() {
    currentProduct_.clear();
    lblBarcode_->setText("--");
    lblName_->setText("--");
    lblName_->setStyleSheet("");
    lblSpec_->setText("--");
    lblPrice_->setText("--");
    lblStock_->setText("--");
    spinQty_->setValue(1);
    editRemark_->clear();
    btnOk_->setEnabled(false);
}
 
void StockInDialog::onConfirmClicked() {
    if (currentProduct_.isEmpty()) return;
    int pid = currentProduct_["id"].toInt();
    QString bc = currentProduct_["barcode"].toString();
    int qty = spinQty_->value();
 
    DbManager &db = DbManager::instance();
    if (!db.updateStock(pid, +qty)) {
        QMessageBox::warning(this, "失败", "更新库存失败");
        return;
    }
    if (!db.insertOperation("in", pid, bc, qty, operatorName_, editRemark_->text())) {
        QMessageBox::warning(this, "警告", "流水记录失败（库存已更新）");
    }
    lblTip_->setText(QString("✓ 入库成功：%1 × %2").arg(currentProduct_["name"].toString()).arg(qty));
    lblTip_->setStyleSheet("color:#388E3C; padding:4px;");
    clearProduct();
}
