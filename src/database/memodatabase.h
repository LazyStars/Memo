#pragma once

#include <qsqldatabase.h>
#include <qstring.h>

class MemoDatabase
{
public:
    static bool initialize();
    static QSqlDatabase database();
    static QString databasePath();
    static QString lastError();

private:
    MemoDatabase();
    ~MemoDatabase();

    static MemoDatabase& instance();
    bool initializeInternal();
    bool openDatabase();
    bool createSchema();
    bool createMemoGroupTable();
    bool createMemoRecordTable();
    bool createMemoReminderTable();
    bool ensureMemoRecordGroupIdColumn();
    bool ensureMemoReminderColumns();
    bool ensureSingleDefaultGroup(qint64* defaultGroupId);
    bool backfillRecordsToDefaultGroup(qint64 defaultGroupId);
    bool createIndexes();
    bool columnExists(const QString& tableName, const QString& columnName) const;
    QString createConnectionName() const;
    QString resolveDatabasePath() const;
    qint64 currentTimestamp() const;

private:
    QString connectionName;  // 当前数据库连接名
    QString dbPath;          // SQLite 数据库文件绝对路径
    QSqlDatabase db;         // 当前数据库连接
    QString errorMessage;    // 最近一次数据库初始化或连接错误
    bool isInitialized = false; // 是否已完成建库建表初始化
};
