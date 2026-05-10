#ifndef INVENTORYDIALOG_H
#define INVENTORYDIALOG_H

#include <QDialog>

class QTableWidget;
class QLineEdit;

class InventoryDialog : public QDialog {
    Q_OBJECT
public:
    explicit InventoryDialog(QWidget *parent = nullptr);

private slots:
    void refresh();

private:
    QLineEdit    *editKeyword_;
    QTableWidget *table_;
};

#endif // INVENTORYDIALOG_H
