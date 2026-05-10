#include "logindialog.h"
#include "dbmanager.h"

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>

LoginDialog::LoginDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("仓库管理系统 - 登录");
    setFixedSize(360, 220);

    QLabel *title = new QLabel("仓库管理系统");
    QFont f = title->font();
    f.setPointSize(16); f.setBold(true);
    title->setFont(f);
    title->setAlignment(Qt::AlignCenter);

    editUser_ = new QLineEdit;
    editUser_->setPlaceholderText("用户名");
    editUser_->setText("admin");                  // 方便演示，预填

    editPwd_ = new QLineEdit;
    editPwd_->setPlaceholderText("密码");
    editPwd_->setEchoMode(QLineEdit::Password);
    editPwd_->setText("123456");

    QFormLayout *form = new QFormLayout;
    form->addRow("用户名:", editUser_);
    form->addRow("密  码:", editPwd_);

    lblTip_ = new QLabel;
    lblTip_->setStyleSheet("color: red;");

    QPushButton *btnOk     = new QPushButton("登录");
    QPushButton *btnCancel = new QPushButton("退出");
    QHBoxLayout *hb = new QHBoxLayout;
    hb->addStretch(); hb->addWidget(btnOk); hb->addWidget(btnCancel);

    QVBoxLayout *vb = new QVBoxLayout(this);
    vb->addWidget(title);
    vb->addLayout(form);
    vb->addWidget(lblTip_);
    vb->addStretch();
    vb->addLayout(hb);

    connect(btnOk,     &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
}

void LoginDialog::onLoginClicked() {
    QString u = editUser_->text().trimmed();
    QString p = editPwd_->text();
    if (u.isEmpty() || p.isEmpty()) {
        lblTip_->setText("请输入用户名和密码");
        return;
    }
    QString role;
    if (DbManager::instance().checkLogin(u, p, role)) {
        username_ = u;
        role_     = role;
        accept();
    } else {
        lblTip_->setText("用户名或密码错误");
    }
}
