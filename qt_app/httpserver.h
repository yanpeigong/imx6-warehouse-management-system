#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QByteArray>
#include <QString>
#include <QMap>

/**
 * 极简 HTTP 服务器
 * - 只支持 GET / POST
 * - 响应 JSON
 * - 静态文件服务（用于直接 serve index.html）
 *
 * 接口（与原 CGI 完全等价）：
 *   GET  /                            -> index.html
 *   GET  /api?action=stats
 *   GET  /api?action=products&kw=...
 *   GET  /api?action=ops
 *   GET  /api?action=chart
 *   POST /api?action=add_product       (body: JSON)
 */
class HttpServer : public QObject {
    Q_OBJECT
public:
    explicit HttpServer(QObject *parent = nullptr);

    // 启动监听
    bool start(quint16 port = 8080);
    quint16 port() const { return port_; }

    // 设置静态文件目录（存放 index.html 等）
    void setWebRoot(const QString &dir) { webRoot_ = dir; }

private slots:
    void onNewConnection();
    void onReadyRead();
    void onDisconnected();

private:
    // 处理一次完整请求，返回响应字符串
    QByteArray handleRequest(const QString &method, const QString &path,
                             const QMap<QString,QString> &query, const QByteArray &body);

    // ===== 各接口的实现（和原 CGI 一一对应）=====
    QByteArray apiStats();
    QByteArray apiProducts(const QString &kw);
    QByteArray apiOps();
    QByteArray apiChart();
    QByteArray apiAddProduct(const QByteArray &body);

    // 工具：构造 HTTP 响应
    QByteArray makeResponse(int code, const QByteArray &contentType, const QByteArray &body);
    QByteArray makeJsonResponse(int code, const QByteArray &json);

    // 工具：解析 query string（简单 URL 解码）
    QMap<QString,QString> parseQuery(const QString &qs);
    QString urlDecode(const QString &s);

    // 静态文件
    QByteArray serveStaticFile(const QString &path);

    QTcpServer *server_;
    quint16     port_;
    QString     webRoot_;

    // 每个连接的缓冲区（可能分包到达）
    QMap<QTcpSocket*, QByteArray> buffers_;
};

#endif // HTTPSERVER_H
