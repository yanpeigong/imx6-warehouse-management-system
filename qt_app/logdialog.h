#ifndef LOGDIALOG_H
#define LOGDIALOG_H

#include <QDialog>

class QTableWidget;

class LogDialog : public QDialog {
    Q_OBJECT
public:
    explicit LogDialog(QWidget *parent = nullptr);

private slots:
    void refresh();

private:
    QTableWidget *table_;
};

#endif // LOGDIALOG_H
