#ifndef STOCKOUTDIALOG_H
#define STOCKOUTDIALOG_H

#include <QDialog>
#include <QString>
#include <QVariantMap>

class QLabel;
class QLineEdit;
class QSpinBox;
class QPushButton;
class SerialReader;

class StockOutDialog : public QDialog {
    Q_OBJECT
public:
    explicit StockOutDialog(SerialReader *reader, const QString &operatorName,
                            QWidget *parent = nullptr);

private slots:
    void onBarcodeScanned(const QString &barcode);
    void onConfirmClicked();
    void onManualInputClicked();

private:
    void loadProduct(const QString &barcode);
    void clearProduct();

    SerialReader *reader_;
    QString operatorName_;
    QVariantMap currentProduct_;

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

#endif // STOCKOUTDIALOG_H
