#include "ConnectionPool.h"
#include <QDebug>
#include <QtSql>
#include <QStack>
#include <QString>
#include <QMutex>
#include <QSemaphore>

////////////////////////////////////////////////////////////////////////////////
///                                                                          ///
///                         ConnectionPoolPrivate 的定义                      ///
///                                                                          ///
////////////////////////////////////////////////////////////////////////////////
class ConnectionPoolPrivate {
public:
    ConnectionPoolPrivate();
    ~ConnectionPoolPrivate();

    QSqlDatabase createConnection(const QString &connectionName); // 创建数据库连接

    QStack<QString> usedConnectionNames;   // 已使用的数据库连接名
    QStack<QString> unusedConnectionNames; // 未使用的数据库连接名

    // 数据库信息
    QString hostName;
    QString databaseName;
    QString username;
    QString password;
    QString databaseType;
    int port;

    bool    testOnBorrow;    // 取得连接的时候验证连接有效
    QString testOnBorrowSql; // 测试访问数据库的 SQL
    int maxWaitTime;         // 获取连接最大等待时间
    int maxConnectionCount;  // 最大连接数

    QSemaphore *semaphore;

    static QMutex mutex;
    static int lastKey; // 用来创建连接的名字，保证连接名字不会重复
};

QMutex ConnectionPoolPrivate::mutex;
int ConnectionPoolPrivate::lastKey = 0;

ConnectionPoolPrivate::ConnectionPoolPrivate() {
    // 创建数据库连接的这些信息在实际开发的时都需要通过读取配置文件得到，
    // 这里为了演示方便所以写死在了代码里。
    hostName     = "127.0.0.1";
    databaseName = "qt";
    username     = "root";
    password     = "root";
    databaseType = "QPSQL";//QMYSQL
    port         = 0;//3306
    testOnBorrow = true;
    testOnBorrowSql = "SELECT 1";
    maxWaitTime     = 5000;
    maxConnectionCount = 5;

    semaphore = new QSemaphore(maxConnectionCount);
}

ConnectionPoolPrivate::~ConnectionPoolPrivate() {
    // 销毁连接池的时候删除所有的连接
    foreach(QString connectionName, usedConnectionNames) {
        createConnection(connectionName).close();
        QSqlDatabase::removeDatabase(connectionName);
    }

    foreach(QString connectionName, unusedConnectionNames) {
        QSqlDatabase::removeDatabase(connectionName);
    }

    delete semaphore;
}

QSqlDatabase ConnectionPoolPrivate::createConnection(const QString &connectionName) {
    // 连接已经创建过了，复用它，而不是重新创建
    if (QSqlDatabase::contains(connectionName)) {
        QSqlDatabase db1 = QSqlDatabase::database(connectionName);

        if (testOnBorrow) {
            // 返回连接前访问数据库，如果连接断开，重新建立连接
            qDebug() << QString("Test connection on borrow, execute: %1, for connection %2")
                        .arg(testOnBorrowSql).arg(connectionName);
            QSqlQuery query(testOnBorrowSql, db1);

            if (query.lastError().type() != QSqlError::NoError && !db1.open()) {
                qDebug() << "Open datatabase error:" << db1.lastError().text();
                return QSqlDatabase();
            }
        }

        return db1;
    }

    // 创建一个新的连接
    QSqlDatabase db = QSqlDatabase::addDatabase(databaseType, connectionName);
    db.setHostName(hostName);
    db.setDatabaseName(databaseName);
    db.setUserName(username);
    db.setPassword(password);

    if (port != 0) {
        db.setPort(port);
    }

    if (!db.open()) {
        qDebug() << "Open datatabase error:" << db.lastError().text();
        return QSqlDatabase();
    }

    return db;
}

////////////////////////////////////////////////////////////////////////////////
///                                                                          ///
///                           ConnectionPool 的定义                           ///
///                                                                          ///
////////////////////////////////////////////////////////////////////////////////
ConnectionPool* ConnectionPool::instance = NULL;

ConnectionPool::ConnectionPool() {
    data = new ConnectionPoolPrivate();
}

ConnectionPool::~ConnectionPool() {
    delete data;
}

ConnectionPool& ConnectionPool::getInstance() {
    if (NULL == instance) {
        QMutexLocker locker(&ConnectionPoolPrivate::mutex);

        if (NULL == instance) {
            instance = new ConnectionPool();
        }
    }

    return *instance;
}

void ConnectionPool::release() {
    QMutexLocker locker(&ConnectionPoolPrivate::mutex);
    delete instance;
    instance = NULL;
}

void ConnectionPool::initDb(const QString& serveraddress, const QString& database, const QString& uid, const QString& pwd)
{
    ConnectionPool& pool = ConnectionPool::getInstance();
     pool.data->hostName = serveraddress;
     pool.data->databaseName = database;
     pool.data->username = uid;
     pool.data->password = pwd;
}

QSqlDatabase ConnectionPool::openConnection() {
    ConnectionPool& pool = ConnectionPool::getInstance();

    if (pool.data->semaphore->tryAcquire(1, pool.data->maxWaitTime)) {
        // 有已经回收的连接，复用它们
        // 没有已经回收的连接，则创建新的连接
        ConnectionPoolPrivate::mutex.lock();
        QString connectionName = pool.data->unusedConnectionNames.size() > 0 ?
                    pool.data->unusedConnectionNames.pop() :
                    QString("C%1").arg(++ConnectionPoolPrivate::lastKey);
        pool.data->usedConnectionNames.push_back(connectionName);
        ConnectionPoolPrivate::mutex.unlock();

        // 创建连接。因为创建连接很耗时，所以不放在 lock 的范围内，提高并发效率
        QSqlDatabase db = pool.data->createConnection(connectionName);

        if (!db.isOpen()) {
            ConnectionPoolPrivate::mutex.lock();
            int ersId = pool.data->usedConnectionNames.indexOf(connectionName);
            if(ersId!=-1)
                pool.data->usedConnectionNames.remove(ersId); // 无效连接删除
            ConnectionPoolPrivate::mutex.unlock();

            pool.data->semaphore->release(); // 没有消耗连接
        }

        return db;
    } else {
        // 创建连接超时，返回一个无效连接
        qDebug() << "Time out to create connection.";
        return QSqlDatabase();
    }
}

void ConnectionPool::closeConnection(const QSqlDatabase &connection) {
    ConnectionPool& pool = ConnectionPool::getInstance();
    QString connectionName = connection.connectionName();

    // 如果是我们创建的连接，并且已经被使用，则从 used 里删除，放入 unused 里
    int connectNameId = pool.data->usedConnectionNames.indexOf(connectionName);
    if (connectNameId!=-1) {//pool.data->usedConnectionNames.contains(connectionName)

        QMutexLocker locker(&ConnectionPoolPrivate::mutex);
        pool.data->usedConnectionNames.remove(connectNameId);
        pool.data->unusedConnectionNames.push_back(connectionName);
        pool.data->semaphore->release();
    }
}
