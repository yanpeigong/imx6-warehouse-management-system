#include "dbmanager.h"
#include <QDebug>

DbManager& DbManager::instance() {
    static DbManager inst;
    return inst;
}

DbManager::DbManager(QObject *parent) : QObject(parent), db_(nullptr) {}

bool DbManager::init(const QString &dbPath) {
    int rc = sqlite3_open(dbPath.toUtf8().constData(), &db_);
    if (rc != SQLITE_OK) {
        qWarning() << "数据库打开失败:" << sqlite3_errmsg(db_);
        return false;
    }
    sqlite3_exec(db_, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
    sqlite3_exec(db_, "PRAGMA foreign_keys=ON;", nullptr, nullptr, nullptr);
    qDebug() << "数据库已连接:" << dbPath;
    return true;
}

// ============== 用户 ==============
bool DbManager::checkLogin(const QString &username, const QString &password, QString &outRole) {
    sqlite3_stmt *stmt = nullptr;
    const char *sql = "SELECT role FROM users WHERE username=? AND password=?";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        qWarning() << "登录SQL编译失败:" << sqlite3_errmsg(db_);
        return false;
    }
    sqlite3_bind_text(stmt, 1, username.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, password.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    bool ok = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        outRole = QString::fromUtf8((const char*)sqlite3_column_text(stmt, 0));
        ok = true;
    }
    sqlite3_finalize(stmt);
    return ok;
}

// ============== 商品 ==============
QVariantMap DbManager::findProductByBarcode(const QString &barcode) {
    QVariantMap m;
    sqlite3_stmt *stmt = nullptr;
    const char *sql = "SELECT id,barcode,name,spec,price,stock FROM products WHERE barcode=?";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return m;
    sqlite3_bind_text(stmt, 1, barcode.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        m["id"]      = sqlite3_column_int(stmt, 0);
        m["barcode"] = QString::fromUtf8((const char*)sqlite3_column_text(stmt, 1));
        m["name"]    = QString::fromUtf8((const char*)sqlite3_column_text(stmt, 2));
        m["spec"]    = QString::fromUtf8((const char*)sqlite3_column_text(stmt, 3));
        m["price"]   = sqlite3_column_double(stmt, 4);
        m["stock"]   = sqlite3_column_int(stmt, 5);
    }
    sqlite3_finalize(stmt);
    return m;
}

int DbManager::addProduct(const QString &barcode, const QString &name,
                          const QString &spec, double price) {
    sqlite3_stmt *stmt = nullptr;
    const char *sql = "INSERT INTO products(barcode,name,spec,price,stock) VALUES(?,?,?,?,0)";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        qWarning() << "新增商品SQL编译失败:" << sqlite3_errmsg(db_);
        return -1;
    }
    sqlite3_bind_text(stmt, 1, barcode.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, name.toUtf8().constData(),    -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, spec.toUtf8().constData(),    -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 4, price);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        qWarning() << "新增商品失败:" << sqlite3_errmsg(db_);
        return -1;
    }
    return (int)sqlite3_last_insert_rowid(db_);
}

bool DbManager::updateStock(int productId, int delta) {
    sqlite3_stmt *stmt = nullptr;
    const char *sql = "UPDATE products SET stock = stock + ? WHERE id=?";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, delta);
    sqlite3_bind_int(stmt, 2, productId);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

QList<QVariantMap> DbManager::listAllProducts(const QString &keyword) {
    QList<QVariantMap> list;
    sqlite3_stmt *stmt = nullptr;
    if (keyword.isEmpty()) {
        const char *sql = "SELECT id,barcode,name,spec,price,stock FROM products ORDER BY id";
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return list;
    } else {
        const char *sql = "SELECT id,barcode,name,spec,price,stock FROM products "
                          "WHERE barcode LIKE ? OR name LIKE ? ORDER BY id";
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return list;
        QString like = "%" + keyword + "%";
        sqlite3_bind_text(stmt, 1, like.toUtf8().constData(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, like.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        QVariantMap m;
        m["id"]      = sqlite3_column_int(stmt, 0);
        m["barcode"] = QString::fromUtf8((const char*)sqlite3_column_text(stmt, 1));
        m["name"]    = QString::fromUtf8((const char*)sqlite3_column_text(stmt, 2));
        const unsigned char *spec = sqlite3_column_text(stmt, 3);
        m["spec"]    = spec ? QString::fromUtf8((const char*)spec) : QString();
        m["price"]   = sqlite3_column_double(stmt, 4);
        m["stock"]   = sqlite3_column_int(stmt, 5);
        list.append(m);
    }
    sqlite3_finalize(stmt);
    return list;
}

// ============== 流水 ==============
bool DbManager::insertOperation(const QString &opType, int productId, const QString &barcode,
                                int quantity, const QString &operator_, const QString &remark) {
    sqlite3_stmt *stmt = nullptr;
    const char *sql = "INSERT INTO operations(op_type,product_id,barcode,quantity,operator,remark) "
                      "VALUES(?,?,?,?,?,?)";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, opType.toUtf8().constData(),    -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 2, productId);
    sqlite3_bind_text(stmt, 3, barcode.toUtf8().constData(),   -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 4, quantity);
    sqlite3_bind_text(stmt, 5, operator_.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, remark.toUtf8().constData(),    -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

QList<QVariantMap> DbManager::recentOperations(int limit) {
    QList<QVariantMap> list;
    sqlite3_stmt *stmt = nullptr;
    const char *sql = "SELECT o.id,o.op_type,o.barcode,p.name,o.quantity,o.operator,o.op_time,o.remark "
                      "FROM operations o LEFT JOIN products p ON o.product_id=p.id "
                      "ORDER BY o.id DESC LIMIT ?";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return list;
    sqlite3_bind_int(stmt, 1, limit);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        QVariantMap m;
        m["id"]       = sqlite3_column_int(stmt, 0);
        m["op_type"]  = QString::fromUtf8((const char*)sqlite3_column_text(stmt, 1));
        m["barcode"]  = QString::fromUtf8((const char*)sqlite3_column_text(stmt, 2));
        const unsigned char *name = sqlite3_column_text(stmt, 3);
        m["name"]     = name ? QString::fromUtf8((const char*)name) : QString();
        m["quantity"] = sqlite3_column_int(stmt, 4);
        const unsigned char *op = sqlite3_column_text(stmt, 5);
        m["operator"] = op ? QString::fromUtf8((const char*)op) : QString();
        m["op_time"]  = QString::fromUtf8((const char*)sqlite3_column_text(stmt, 6));
        const unsigned char *rmk = sqlite3_column_text(stmt, 7);
        m["remark"]   = rmk ? QString::fromUtf8((const char*)rmk) : QString();
        list.append(m);
    }
    sqlite3_finalize(stmt);
    return list;
}

// ============== 给 HTTP 用的统计 ==============
int DbManager::countProducts() {
    sqlite3_stmt *stmt = nullptr;
    int n = 0;
    if (sqlite3_prepare_v2(db_, "SELECT COUNT(*) FROM products", -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) n = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
    }
    return n;
}

int DbManager::sumStock() {
    sqlite3_stmt *stmt = nullptr;
    int n = 0;
    if (sqlite3_prepare_v2(db_, "SELECT COALESCE(SUM(stock),0) FROM products", -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) n = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
    }
    return n;
}

double DbManager::sumStockValue() {
    sqlite3_stmt *stmt = nullptr;
    double v = 0;
    if (sqlite3_prepare_v2(db_, "SELECT COALESCE(SUM(stock*price),0) FROM products", -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) v = sqlite3_column_double(stmt, 0);
        sqlite3_finalize(stmt);
    }
    return v;
}

int DbManager::countOperationsOnDate(const QString &date) {
    sqlite3_stmt *stmt = nullptr;
    int n = 0;
    if (sqlite3_prepare_v2(db_, "SELECT COUNT(*) FROM operations WHERE date(op_time)=?",
                           -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, date.toUtf8().constData(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW) n = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
    }
    return n;
}

int DbManager::countOperationsByTypeOnDate(const QString &opType, const QString &date) {
    sqlite3_stmt *stmt = nullptr;
    int n = 0;
    if (sqlite3_prepare_v2(db_,
            "SELECT COUNT(*) FROM operations WHERE op_type=? AND date(op_time)=?",
            -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, opType.toUtf8().constData(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, date.toUtf8().constData(),   -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW) n = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
    }
    return n;
}

QList<QVariantMap> DbManager::topStockProducts(int limit) {
    QList<QVariantMap> list;
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db_,
            "SELECT name,stock FROM products ORDER BY stock DESC LIMIT ?",
            -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, limit);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            QVariantMap m;
            m["name"]  = QString::fromUtf8((const char*)sqlite3_column_text(stmt, 0));
            m["stock"] = sqlite3_column_int(stmt, 1);
            list.append(m);
        }
        sqlite3_finalize(stmt);
    }
    return list;
}
