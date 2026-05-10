#ifndef STOCKINDIALOG_H
#define STOCKINDIALOG_H

#include <QDialog>
#include <QString>
#include <QVariantMap>

class QLabel;
class QLineEdit;
class QSpinBox;
class QPushButton;
class SerialReader;

/**
 * 入库对话框
 * - 顶部显示提示"请扫描条码"
 * - 收到条码后查商品：找到 -> 显示信息；找不到 -> 弹出新增商品
 * - 用户填数量、备注 -> 点确认 -> 写入流水 + 更新库存
 */
class StockInDialog : public QDialog {
    Q_OBJECT
public:
    explicit StockInDialog(SerialReader *reader, const QString &operatorName,
                           QWidget *parent = nullptr);

private slots:
    void onBarcodeScanned(const QString &barcode);
    void onConfirmClicked();
    void onManualInputClicked();         // 没有扫码枪时手动输入条码

private:
    void loadProduct(const QString &barcode);
    void clearProduct();

    SerialReader *reader_;
    QString operatorName_;
    QVariantMap currentProduct_;         // 当前界面上展示的商品

    QLabel    *lblBarcode_;
    QLabel    *lblName_;
    QLabel    *lblSpec_;
    QLabel    *lblPrice_;
    QLabel    *lblStock_;
    QSpinBox  *spinQty_;
    QLineEdit *editRemark_;
    QPushButton *btnOk_;
    QLabel    *lblTip_;
};

#endif // STOCKINDIALOG_H
