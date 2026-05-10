#ifndef DBMANAGER_H
#define DBMANAGER_H

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QList>
#include <sqlite3.h>

/**
 * 数据库管理类（单例）
 * 直接使用 sqlite3 C API，不依赖 Qt SQL 模块（避开 libqsqlite.so plugin 问题）
 */
class DbManager : public QObject {
    Q_OBJECT
public:
    static DbManager& instance();

    bool init(const QString &dbPath);

    // ========== 用户相关 ==========
    bool checkLogin(const QString &username, const QString &password, QString &outRole);

    // ========== 商品相关 ==========
    QVariantMap findProductByBarcode(const QString &barcode);
    int  addProduct(const QString &barcode, const QString &name,
                    const QString &spec, double price);
    bool updateStock(int productId, int delta);
    QList<QVariantMap> listAllProducts(const QString &keyword = QString());

    // ========== 流水相关 ==========
    bool insertOperation(const QString &opType, int productId, const QString &barcode,
                         int quantity, const QString &operator_, const QString &remark);
    QList<QVariantMap> recentOperations(int limit = 100);

    // ========== 给 HTTP 服务用的统计接口 ==========
    int    countProducts();
    int    sumStock();
    double sumStockValue();
    int    countOperationsOnDate(const QString &date);
    int    countOperationsByTypeOnDate(const QString &opType, const QString &date);
    QList<QVariantMap> topStockProducts(int limit = 10);

private:
    explicit DbManager(QObject *parent = nullptr);
    DbManager(const DbManager&) = delete;
    DbManager& operator=(const DbManager&) = delete;

    sqlite3 *db_;
};

#endif // DBMANAGER_H
