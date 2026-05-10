#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QString>

class QLineEdit;
class QLabel;

class LoginDialog : public QDialog {
    Q_OBJECT
public:
    explicit LoginDialog(QWidget *parent = nullptr);

    QString username() const { return username_; }
    QString role()     const { return role_; }

private slots:
    void onLoginClicked();

private:
    QLineEdit *editUser_;
    QLineEdit *editPwd_;
    QLabel    *lblTip_;
    QString    username_;
    QString    role_;
};

#endif // LOGINDIALOG_H
