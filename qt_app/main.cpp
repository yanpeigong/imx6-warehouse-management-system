#include "mainwindow.h"
#include "logindialog.h"
#include "dbmanager.h"
#include "httpserver.h"

#include <QApplication>
#include <QMessageBox>
#include <QFile>
#include <QDir>
#include <QTextCodec>
#include <QNetworkInterface>

// ====== 数据库文件路径 ======
// 实验箱上建议放在 /opt/warehouse/warehouse.db
// 这里用相对路径，方便开发调试
static const QString DB_PATH    = "warehouse.db";
// Web 文件目录：HTTP 服务器从这里读 index.html
static const QString WEB_ROOT   = "./web";
// HTTP 服务端口
static const quint16 HTTP_PORT  = 8080;

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // 全局中文支持
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));

    // 设置全局字体（解决一些嵌入式 Linux 上中文显示问题）
    QFont f = app.font();
    f.setPointSize(11);
    app.setFont(f);

    // 检查数据库文件是否存在
    if (!QFile::exists(DB_PATH)) {
        QMessageBox::critical(nullptr, "错误",
            QString("数据库文件不存在: %1\n请先用 init_db.sql 初始化数据库")
                .arg(QDir::current().absoluteFilePath(DB_PATH)));
        return 1;
    }
    if (!DbManager::instance().init(DB_PATH)) {
        QMessageBox::critical(nullptr, "错误", "数据库连接失败！");
        return 1;
    }

    // 启动内置 HTTP 服务器（Web 端通过此端口访问）
    HttpServer *http = new HttpServer(&app);
    http->setWebRoot(WEB_ROOT);
    if (http->start(HTTP_PORT)) {
        // 打印本机所有网络接口的 IP，方便老师在主机上访问
        qDebug() << "===== Web 端访问地址 =====";
        foreach (const QHostAddress &addr, QNetworkInterface::allAddresses()) {
            if (addr.protocol() == QAbstractSocket::IPv4Protocol
                && addr != QHostAddress::LocalHost) {
                qDebug() << QString("  http://%1:%2/").arg(addr.toString()).arg(HTTP_PORT);
            }
        }
    } else {
        qWarning() << "HTTP 服务启动失败，Web 端将无法访问";
    }

    // 登录
    LoginDialog login;
    if (login.exec() != QDialog::Accepted) return 0;

    // 主窗口
    MainWindow w(login.username(), login.role());
    w.show();
    return app.exec();
}
