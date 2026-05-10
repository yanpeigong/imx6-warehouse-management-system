#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>

class SerialReader;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(const QString &username, const QString &role,
                        QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onStockIn();
    void onStockOut();
    void onInventory();
    void onLog();
    void onSerialError(const QString &msg);

private:
    QString username_;
    QString role_;
    SerialReader *reader_;
};

#endif // MAINWINDOW_H
