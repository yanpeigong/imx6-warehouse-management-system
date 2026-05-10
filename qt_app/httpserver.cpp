#include "httpserver.h"
#include "dbmanager.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QDebug>
#include <QStringList>

HttpServer::HttpServer(QObject *parent)
    : QObject(parent), server_(new QTcpServer(this)), port_(0) {
    connect(server_, &QTcpServer::newConnection,
            this,    &HttpServer::onNewConnection);
}

bool HttpServer::start(quint16 port) {
    if (!server_->listen(QHostAddress::Any, port)) {
        qWarning() << "HttpServer 启动失败:" << server_->errorString();
        return false;
    }
    port_ = server_->serverPort();
    qDebug() << "HttpServer 已监听端口:" << port_;
    return true;
}

void HttpServer::onNewConnection() {
    while (server_->hasPendingConnections()) {
        QTcpSocket *sock = server_->nextPendingConnection();
        connect(sock, &QTcpSocket::readyRead,    this, &HttpServer::onReadyRead);
        connect(sock, &QTcpSocket::disconnected, this, &HttpServer::onDisconnected);
        buffers_[sock] = QByteArray();
    }
}

void HttpServer::onDisconnected() {
    QTcpSocket *sock = qobject_cast<QTcpSocket*>(sender());
    if (!sock) return;
    buffers_.remove(sock);
    sock->deleteLater();
}

void HttpServer::onReadyRead() {
    QTcpSocket *sock = qobject_cast<QTcpSocket*>(sender());
    if (!sock) return;

    buffers_[sock].append(sock->readAll());
    QByteArray &buf = buffers_[sock];

    // 找头部结束位置
    int headerEnd = buf.indexOf("\r\n\r\n");
    if (headerEnd < 0) return;          // 头部还没收完

    QByteArray headerPart = buf.left(headerEnd);
    QList<QByteArray> lines = headerPart.split('\n');
    if (lines.isEmpty()) { sock->close(); return; }

    // 第一行：METHOD PATH HTTP/1.x
    QList<QByteArray> firstLine = lines[0].trimmed().split(' ');
    if (firstLine.size() < 2) { sock->close(); return; }
    QString method = QString::fromLatin1(firstLine[0]);
    QString fullPath = QString::fromLatin1(firstLine[1]);

    // 解析 Content-Length
    int contentLength = 0;
    for (int i = 1; i < lines.size(); ++i) {
        QByteArray line = lines[i].trimmed();
        if (line.toLower().startsWith("content-length:")) {
            contentLength = line.mid(15).trimmed().toInt();
            break;
        }
    }

    // 检查 body 是否完整
    int bodyAvail = buf.size() - (headerEnd + 4);
    if (bodyAvail < contentLength) return;          // body 还没收完，等
    QByteArray body = buf.mid(headerEnd + 4, contentLength);

    // 拆 path 和 query
    QString path = fullPath;
    QString queryStr;
    int qIdx = fullPath.indexOf('?');
    if (qIdx >= 0) {
        path = fullPath.left(qIdx);
        queryStr = fullPath.mid(qIdx + 1);
    }
    QMap<QString,QString> query = parseQuery(queryStr);

    // 处理请求
    QByteArray resp = handleRequest(method, path, query, body);
    sock->write(resp);
    sock->flush();
    sock->disconnectFromHost();
    buffers_[sock].clear();
}

QByteArray HttpServer::handleRequest(const QString &method, const QString &path,
                                     const QMap<QString,QString> &query, const QByteArray &body) {
    // 静态文件
    if (path == "/" || path == "/index.html") {
        return serveStaticFile("/index.html");
    }
    // 让 favicon 和其它静态资源也能 serve（例如本地 bootstrap.css）
    if (!path.startsWith("/api")) {
        return serveStaticFile(path);
    }

    // API
    QString action = query.value("action");
    if (action == "stats")        return apiStats();
    if (action == "products")     return apiProducts(query.value("kw"));
    if (action == "ops")          return apiOps();
    if (action == "chart")        return apiChart();
    if (action == "add_product" && method == "POST") return apiAddProduct(body);

    QJsonObject obj;
    obj["ok"] = false;
    obj["msg"] = QString("unknown action: %1").arg(action);
    return makeJsonResponse(404, QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

// ============== API 实现 ==============
QByteArray HttpServer::apiStats() {
    DbManager &db = DbManager::instance();
    int    products    = db.countProducts();
    int    stock       = db.sumStock();
    double value       = db.sumStockValue();
    QString today      = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    int    todayCount  = db.countOperationsOnDate(today);

    QJsonObject obj;
    obj["products"] = products;
    obj["stock"]    = stock;
    obj["value"]    = value;
    obj["today"]    = todayCount;
    return makeJsonResponse(200, QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

QByteArray HttpServer::apiProducts(const QString &kw) {
    auto list = DbManager::instance().listAllProducts(kw);
    QJsonArray arr;
    foreach (const QVariantMap &m, list) {
        QJsonObject o;
        o["id"]      = m["id"].toInt();
        o["barcode"] = m["barcode"].toString();
        o["name"]    = m["name"].toString();
        o["spec"]    = m["spec"].toString();
        o["price"]   = m["price"].toDouble();
        o["stock"]   = m["stock"].toInt();
        arr.append(o);
    }
    QJsonObject obj; obj["data"] = arr;
    return makeJsonResponse(200, QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

QByteArray HttpServer::apiOps() {
    auto list = DbManager::instance().recentOperations(100);
    QJsonArray arr;
    foreach (const QVariantMap &m, list) {
        QJsonObject o;
        o["id"]       = m["id"].toInt();
        o["op_type"]  = m["op_type"].toString();
        o["barcode"]  = m["barcode"].toString();
        o["name"]     = m["name"].toString();
        o["quantity"] = m["quantity"].toInt();
        o["operator"] = m["operator"].toString();
        o["op_time"]  = m["op_time"].toString();
        o["remark"]   = m["remark"].toString();
        arr.append(o);
    }
    QJsonObject obj; obj["data"] = arr;
    return makeJsonResponse(200, QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

QByteArray HttpServer::apiChart() {
    DbManager &db = DbManager::instance();

    // 库存 Top 10
    auto topList = db.topStockProducts(10);
    QJsonArray sLabels, sValues;
    foreach (const QVariantMap &m, topList) {
        sLabels.append(m["name"].toString());
        sValues.append(m["stock"].toInt());
    }
    QJsonObject stockObj;
    stockObj["labels"] = sLabels;
    stockObj["values"] = sValues;

    // 近 7 日趋势
    QJsonArray tLabels, tIn, tOut;
    QDate today = QDate::currentDate();
    for (int i = 6; i >= 0; --i) {
        QDate d = today.addDays(-i);
        QString ds = d.toString("yyyy-MM-dd");
        tLabels.append(d.toString("MM-dd"));
        tIn.append(db.countOperationsByTypeOnDate("in",  ds));
        tOut.append(db.countOperationsByTypeOnDate("out", ds));
    }
    QJsonObject trendObj;
    trendObj["labels"]     = tLabels;
    trendObj["in_values"]  = tIn;
    trendObj["out_values"] = tOut;

    QJsonObject obj;
    obj["stock"] = stockObj;
    obj["trend"] = trendObj;
    return makeJsonResponse(200, QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

QByteArray HttpServer::apiAddProduct(const QByteArray &body) {
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(body, &err);
    if (err.error != QJsonParseError::NoError) {
        QJsonObject o; o["ok"] = false; o["msg"] = "invalid json";
        return makeJsonResponse(400, QJsonDocument(o).toJson(QJsonDocument::Compact));
    }
    QJsonObject in = doc.object();
    QString barcode = in.value("barcode").toString().trimmed();
    QString name    = in.value("name").toString().trimmed();
    QString spec    = in.value("spec").toString().trimmed();
    double  price   = in.value("price").toDouble();

    if (barcode.isEmpty() || name.isEmpty()) {
        QJsonObject o; o["ok"] = false; o["msg"] = "barcode and name required";
        return makeJsonResponse(400, QJsonDocument(o).toJson(QJsonDocument::Compact));
    }
    // 重复检查
    if (!DbManager::instance().findProductByBarcode(barcode).isEmpty()) {
        QJsonObject o; o["ok"] = false; o["msg"] = "barcode exists";
        return makeJsonResponse(409, QJsonDocument(o).toJson(QJsonDocument::Compact));
    }
    int id = DbManager::instance().addProduct(barcode, name, spec, price);
    QJsonObject o;
    if (id > 0) {
        o["ok"] = true; o["id"] = id;
        return makeJsonResponse(200, QJsonDocument(o).toJson(QJsonDocument::Compact));
    } else {
        o["ok"] = false; o["msg"] = "db error";
        return makeJsonResponse(500, QJsonDocument(o).toJson(QJsonDocument::Compact));
    }
}

// ============== 工具 ==============
QByteArray HttpServer::makeResponse(int code, const QByteArray &contentType, const QByteArray &body) {
    QByteArray status;
    switch (code) {
        case 200: status = "200 OK";                    break;
        case 400: status = "400 Bad Request";           break;
        case 404: status = "404 Not Found";             break;
        case 409: status = "409 Conflict";              break;
        case 500: status = "500 Internal Server Error"; break;
        default:  status = QByteArray::number(code) + " OK";
    }
    QByteArray resp;
    resp += "HTTP/1.1 " + status + "\r\n";
    resp += "Content-Type: " + contentType + "\r\n";
    resp += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
    resp += "Cache-Control: no-cache\r\n";
    resp += "Access-Control-Allow-Origin: *\r\n";   // 允许跨域，调试时方便
    resp += "Connection: close\r\n";
    resp += "\r\n";
    resp += body;
    return resp;
}

QByteArray HttpServer::makeJsonResponse(int code, const QByteArray &json) {
    return makeResponse(code, "application/json; charset=utf-8", json);
}

QMap<QString,QString> HttpServer::parseQuery(const QString &qs) {
    QMap<QString,QString> m;
    if (qs.isEmpty()) return m;
    QStringList parts = qs.split('&');
    foreach (const QString &p, parts) {
        int eq = p.indexOf('=');
        if (eq < 0) {
            m[urlDecode(p)] = "";
        } else {
            m[urlDecode(p.left(eq))] = urlDecode(p.mid(eq + 1));
        }
    }
    return m;
}

QString HttpServer::urlDecode(const QString &s) {
    return QUrl::fromPercentEncoding(s.toUtf8());
}

QByteArray HttpServer::serveStaticFile(const QString &path) {
    if (webRoot_.isEmpty()) {
        return makeResponse(404, "text/plain", "web root not set");
    }
    // 防止路径穿越
    QString safe = path;
    safe.replace("..", "");
    QString fullPath = webRoot_ + safe;
    QFile f(fullPath);
    if (!f.exists() || !f.open(QIODevice::ReadOnly)) {
        return makeResponse(404, "text/plain", "not found: " + path.toUtf8());
    }
    QByteArray content = f.readAll();
    f.close();

    // 简单 MIME 推断
    QByteArray mime = "application/octet-stream";
    if (path.endsWith(".html")) mime = "text/html; charset=utf-8";
    else if (path.endsWith(".css"))  mime = "text/css; charset=utf-8";
    else if (path.endsWith(".js"))   mime = "application/javascript; charset=utf-8";
    else if (path.endsWith(".json")) mime = "application/json; charset=utf-8";
    else if (path.endsWith(".png"))  mime = "image/png";
    else if (path.endsWith(".jpg") || path.endsWith(".jpeg")) mime = "image/jpeg";
    else if (path.endsWith(".ico"))  mime = "image/x-icon";
    return makeResponse(200, mime, content);
}
