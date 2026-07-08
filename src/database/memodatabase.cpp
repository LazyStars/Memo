#include "memodatabase.h"

#include <qdatetime.h>
#include <qcoreapplication.h>
#include <qdir.h>
#include <qlist.h>
#include <qsqlerror.h>
#include <qsqlquery.h>
#include <qstandardpaths.h>
#include <quuid.h>
#include <qvariant.h>

MemoDatabase::MemoDatabase()
{
    connectionName = createConnectionName();
    dbPath = resolveDatabasePath();
}

MemoDatabase::~MemoDatabase() {
    if (db.isValid()) {
        db.close();
    }
    db = QSqlDatabase();
    QSqlDatabase::removeDatabase(connectionName);
}

MemoDatabase& MemoDatabase::instance() {
    static MemoDatabase memoDatabase;
    return memoDatabase;
}

bool MemoDatabase::initialize() {
    return instance().initializeInternal();
}

QSqlDatabase MemoDatabase::database() {
    return instance().db;
}

QString MemoDatabase::databasePath() {
    return instance().dbPath;
}

QString MemoDatabase::lastError() {
    return instance().errorMessage;
}

bool MemoDatabase::initializeInternal() {
    if (isInitialized) {
        return true;
    }

    if (!openDatabase()) {
        return false;
    }

    if (!createSchema()) {
        return false;
    }

    isInitialized = true;
    return true;
}

bool MemoDatabase::openDatabase() {
    if (db.isValid() && db.isOpen()) {
        return true;
    }

    if (!db.isValid()) {
        db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName(dbPath);
    }

    if (!db.open()) {
        errorMessage = db.lastError().text();
        return false;
    }

    QSqlQuery query(db);
    if (!query.exec("PRAGMA foreign_keys = ON")) {
        errorMessage = query.lastError().text();
        return false;
    }

    errorMessage.clear();
    return true;
}

bool MemoDatabase::createSchema() {
    if (!createMemoGroupTable()) {
        return false;
    }

    if (!createMemoRecordTable()) {
        return false;
    }

    if (!ensureMemoRecordGroupIdColumn()) {
        return false;
    }

    if (!createMemoReminderTable()) {
        return false;
    }

    qint64 defaultGroupId = 0;
    if (!ensureSingleDefaultGroup(&defaultGroupId)) {
        return false;
    }

    if (!backfillRecordsToDefaultGroup(defaultGroupId)) {
        return false;
    }

    return createIndexes();
}

bool MemoDatabase::createMemoGroupTable() {
    QSqlQuery query(db);
    // memo_group:
    // name: 分组名称
    // sort_order: 分组排序值
    // is_default: 是否为默认分组
    // created_at/updated_at: 创建和更新时间戳
    if (!query.exec(
            "CREATE TABLE IF NOT EXISTS memo_group ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "name TEXT NOT NULL DEFAULT '', "
            "sort_order INTEGER NOT NULL DEFAULT 0, "
            "is_default INTEGER NOT NULL DEFAULT 0, "
            "created_at INTEGER NOT NULL, "
            "updated_at INTEGER NOT NULL"
            ")")) {
        errorMessage = query.lastError().text();
        return false;
    }

    return true;
}

bool MemoDatabase::createMemoRecordTable() {
    QSqlQuery query(db);
    // memo_record:
    // group_id: 所属分组 ID
    // title: 标题
    // content_html: 富文本正文
    // content_plain: 纯文本正文，方便搜索和列表摘要
    // status: 记录状态，对应 MemoStatus
    // plan_due_at: 计划完成绝对时间戳
    // plan_delay_seconds: 原始延迟秒数，便于界面回显
    // completed_at: 实际完成时间戳
    // created_at/updated_at: 创建和更新时间戳
    // deleted: 软删除标记
    if (!query.exec(
            "CREATE TABLE IF NOT EXISTS memo_record ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "group_id INTEGER NOT NULL DEFAULT 0, "
            "title TEXT NOT NULL DEFAULT '', "
            "content_html TEXT NOT NULL DEFAULT '', "
            "content_plain TEXT NOT NULL DEFAULT '', "
            "status INTEGER NOT NULL DEFAULT 1, "
            "plan_due_at INTEGER NOT NULL DEFAULT 0, "
            "plan_delay_seconds INTEGER NOT NULL DEFAULT 0, "
            "completed_at INTEGER NOT NULL DEFAULT 0, "
            "created_at INTEGER NOT NULL, "
            "updated_at INTEGER NOT NULL, "
            "deleted INTEGER NOT NULL DEFAULT 0, "
            "FOREIGN KEY(group_id) REFERENCES memo_group(id) ON DELETE RESTRICT"
            ")")) {
        errorMessage = query.lastError().text();
        return false;
    }

    return true;
}

bool MemoDatabase::ensureMemoRecordGroupIdColumn() {
    if (columnExists("memo_record", "group_id")) {
        return true;
    }

    QSqlQuery query(db);
    if (!query.exec("ALTER TABLE memo_record ADD COLUMN group_id INTEGER NOT NULL DEFAULT 0")) {
        errorMessage = query.lastError().text();
        return false;
    }

    errorMessage.clear();
    return true;
}

bool MemoDatabase::createMemoReminderTable() {
    QSqlQuery query(db);
    // memo_reminder:
    // record_id: 对应 memo_record 的主键，一条记录对应一份提醒配置
    // start_remind_at: 提醒开始工作的时间点
    // due_at: 计划完成时间点
    // finish_within_seconds: 计划需要多久完成
    // remind_interval_seconds: 每次提醒间隔秒数
    // repeat_mode: 单次提醒或重复提醒
    // next_remind_at: 下次应触发提醒的时间点
    // last_reminded_at: 最近一次已提醒时间点
    // is_enabled: 是否启用提醒
    if (!query.exec(
            "CREATE TABLE IF NOT EXISTS memo_reminder ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "record_id INTEGER NOT NULL UNIQUE, "
            "start_remind_at INTEGER NOT NULL DEFAULT 0, "
            "due_at INTEGER NOT NULL DEFAULT 0, "
            "finish_within_seconds INTEGER NOT NULL DEFAULT 0, "
            "remind_interval_seconds INTEGER NOT NULL DEFAULT 0, "
            "repeat_mode INTEGER NOT NULL DEFAULT 0, "
            "next_remind_at INTEGER NOT NULL DEFAULT 0, "
            "last_reminded_at INTEGER NOT NULL DEFAULT 0, "
            "is_enabled INTEGER NOT NULL DEFAULT 1, "
            "created_at INTEGER NOT NULL, "
            "updated_at INTEGER NOT NULL, "
            "FOREIGN KEY(record_id) REFERENCES memo_record(id) ON DELETE CASCADE"
            ")")) {
        errorMessage = query.lastError().text();
        return false;
    }

    return true;
}

bool MemoDatabase::ensureSingleDefaultGroup(qint64* defaultGroupId) {
    if (defaultGroupId == nullptr) {
        errorMessage = "defaultGroupId pointer is null.";
        return false;
    }

    *defaultGroupId = 0;

    QSqlQuery query(db);
    if (!query.exec("SELECT id, is_default FROM memo_group ORDER BY sort_order ASC, id ASC")) {
        errorMessage = query.lastError().text();
        return false;
    }

    QList<qint64> groupIds;
    QList<qint64> defaultIds;
    while (query.next()) {
        const qint64 groupId = query.value("id").toLongLong();
        groupIds.append(groupId);
        if (query.value("is_default").toInt() != 0) {
            defaultIds.append(groupId);
        }
    }

    if (groupIds.isEmpty()) {
        const qint64 now = currentTimestamp();
        QSqlQuery insertQuery(db);
        insertQuery.prepare(
            "INSERT INTO memo_group (name, sort_order, is_default, created_at, updated_at) "
            "VALUES (:name, :sort_order, :is_default, :created_at, :updated_at)");
        insertQuery.bindValue(":name", QStringLiteral("默认分组"));
        insertQuery.bindValue(":sort_order", 0);
        insertQuery.bindValue(":is_default", 1);
        insertQuery.bindValue(":created_at", now);
        insertQuery.bindValue(":updated_at", now);
        if (!insertQuery.exec()) {
            errorMessage = insertQuery.lastError().text();
            return false;
        }

        *defaultGroupId = insertQuery.lastInsertId().toLongLong();
        errorMessage.clear();
        return true;
    }

    *defaultGroupId = defaultIds.isEmpty() ? groupIds.first() : defaultIds.first();

    QSqlQuery updateQuery(db);
    updateQuery.prepare(
        "UPDATE memo_group "
        "SET is_default = CASE WHEN id = :default_id THEN 1 ELSE 0 END");
    updateQuery.bindValue(":default_id", *defaultGroupId);
    if (!updateQuery.exec()) {
        errorMessage = updateQuery.lastError().text();
        return false;
    }

    errorMessage.clear();
    return true;
}

bool MemoDatabase::backfillRecordsToDefaultGroup(qint64 defaultGroupId) {
    if (defaultGroupId <= 0) {
        errorMessage = "default group id is invalid.";
        return false;
    }

    QSqlQuery query(db);
    query.prepare(
        "UPDATE memo_record "
        "SET group_id = :group_id "
        "WHERE group_id = 0 "
        "OR group_id IS NULL "
        "OR NOT EXISTS ("
        "    SELECT 1 FROM memo_group WHERE memo_group.id = memo_record.group_id"
        ")");
    query.bindValue(":group_id", defaultGroupId);
    if (!query.exec()) {
        errorMessage = query.lastError().text();
        return false;
    }

    errorMessage.clear();
    return true;
}

bool MemoDatabase::createIndexes() {
    QSqlQuery query(db);
    if (!query.exec("CREATE INDEX IF NOT EXISTS idx_memo_group_sort_order ON memo_group(sort_order ASC, id ASC)")) {
        errorMessage = query.lastError().text();
        return false;
    }

    if (!query.exec("CREATE INDEX IF NOT EXISTS idx_memo_group_is_default ON memo_group(is_default)")) {
        errorMessage = query.lastError().text();
        return false;
    }

    if (!query.exec("CREATE INDEX IF NOT EXISTS idx_memo_record_group_id ON memo_record(group_id)")) {
        errorMessage = query.lastError().text();
        return false;
    }

    if (!query.exec("CREATE INDEX IF NOT EXISTS idx_memo_record_status ON memo_record(status)")) {
        errorMessage = query.lastError().text();
        return false;
    }

    if (!query.exec("CREATE INDEX IF NOT EXISTS idx_memo_record_updated_at ON memo_record(updated_at DESC)")) {
        errorMessage = query.lastError().text();
        return false;
    }

    if (!query.exec("CREATE INDEX IF NOT EXISTS idx_memo_record_plan_due_at ON memo_record(plan_due_at)")) {
        errorMessage = query.lastError().text();
        return false;
    }

    if (!query.exec("CREATE INDEX IF NOT EXISTS idx_memo_reminder_next_remind_at ON memo_reminder(next_remind_at)")) {
        errorMessage = query.lastError().text();
        return false;
    }

    if (!query.exec("CREATE INDEX IF NOT EXISTS idx_memo_reminder_enabled ON memo_reminder(is_enabled)")) {
        errorMessage = query.lastError().text();
        return false;
    }

    errorMessage.clear();
    return true;
}

QString MemoDatabase::createConnectionName() const {
    return QString("memo_database_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
}

QString MemoDatabase::resolveDatabasePath() const {
    QString basePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (basePath.isEmpty()) {
        basePath = QCoreApplication::applicationDirPath();
    }

    QDir dir(basePath);
    (void)dir.mkpath(".");
    return dir.filePath("memo.db");
}

bool MemoDatabase::columnExists(const QString& tableName, const QString& columnName) const {
    QSqlQuery query(db);
    if (!query.exec(QString("PRAGMA table_info(%1)").arg(tableName))) {
        return false;
    }

    while (query.next()) {
        if (query.value("name").toString() == columnName) {
            return true;
        }
    }

    return false;
}

qint64 MemoDatabase::currentTimestamp() const {
    return QDateTime::currentSecsSinceEpoch();
}
