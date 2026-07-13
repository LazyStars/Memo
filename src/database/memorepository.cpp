#include "memorepository.h"

#include <qsqldatabase.h>
#include <qsqlerror.h>
#include <qsqlquery.h>
#include <qvariant.h>

MemoRepository& MemoRepository::instance() {
    static MemoRepository repository;
    return repository;
}

bool MemoRepository::addGroup(const MemoGroup& group, qint64* insertedId) {
    MemoRepository& repository = instance();
    if (!repository.ensureDatabaseReady()) {
        return false;
    }

    QSqlDatabase database = MemoDatabase::database();
    QSqlQuery query(database);
    query.prepare(
        "INSERT INTO memo_group ("
        "name, sort_order, is_default, created_at, updated_at"
        ") VALUES ("
        ":name, :sort_order, :is_default, :created_at, :updated_at)"
    );
    query.bindValue(":name", group.name);
    query.bindValue(":sort_order", group.sortOrder);
    query.bindValue(":is_default", group.isDefault ? 1 : 0);
    query.bindValue(":created_at", group.createdAt);
    query.bindValue(":updated_at", group.updatedAt);

    if (!query.exec()) {
        repository.errorMessage = query.lastError().text();
        return false;
    }

    if (insertedId != nullptr) {
        *insertedId = query.lastInsertId().toLongLong();
    }

    repository.errorMessage.clear();
    return true;
}

bool MemoRepository::updateGroup(const MemoGroup& group) {
    MemoRepository& repository = instance();
    if (!repository.ensureDatabaseReady()) {
        return false;
    }

    QSqlDatabase database = MemoDatabase::database();
    QSqlQuery query(database);
    query.prepare(
        "UPDATE memo_group "
        "SET name = :name, "
        "sort_order = :sort_order, "
        "is_default = :is_default, "
        "updated_at = :updated_at "
        "WHERE id = :id"
    );
    query.bindValue(":id", group.id);
    query.bindValue(":name", group.name);
    query.bindValue(":sort_order", group.sortOrder);
    query.bindValue(":is_default", group.isDefault ? 1 : 0);
    query.bindValue(":updated_at", group.updatedAt);

    if (!query.exec()) {
        repository.errorMessage = query.lastError().text();
        return false;
    }

    repository.errorMessage.clear();
    return query.numRowsAffected() > 0;
}

bool MemoRepository::deleteGroup(qint64 groupId) {
    MemoRepository& repository = instance();
    if (groupId <= 0) {
        repository.errorMessage = "Memo group id is invalid.";
        return false;
    }

    if (!repository.ensureDatabaseReady()) {
        return false;
    }

    QSqlDatabase database = MemoDatabase::database();
    if (!database.transaction()) {
        repository.errorMessage = database.lastError().text();
        return false;
    }

    QSqlQuery activeRecordQuery(database);
    activeRecordQuery.prepare("SELECT 1 FROM memo_record WHERE group_id = :group_id AND deleted = 0 LIMIT 1");
    activeRecordQuery.bindValue(":group_id", groupId);
    if (!activeRecordQuery.exec()) {
        repository.errorMessage = activeRecordQuery.lastError().text();
        (void)database.rollback();
        return false;
    }

    if (activeRecordQuery.next()) {
        repository.errorMessage = "Memo group contains active records.";
        (void)database.rollback();
        return false;
    }

    // Deleted records no longer belong in a removed group. Deleting them also cascades reminders.
    QSqlQuery removeDeletedRecordsQuery(database);
    removeDeletedRecordsQuery.prepare("DELETE FROM memo_record WHERE group_id = :group_id AND deleted = 1");
    removeDeletedRecordsQuery.bindValue(":group_id", groupId);
    if (!removeDeletedRecordsQuery.exec()) {
        repository.errorMessage = removeDeletedRecordsQuery.lastError().text();
        (void)database.rollback();
        return false;
    }

    QSqlQuery removeGroupQuery(database);
    removeGroupQuery.prepare("DELETE FROM memo_group WHERE id = :group_id");
    removeGroupQuery.bindValue(":group_id", groupId);
    if (!removeGroupQuery.exec()) {
        repository.errorMessage = removeGroupQuery.lastError().text();
        (void)database.rollback();
        return false;
    }

    if (removeGroupQuery.numRowsAffected() != 1) {
        repository.errorMessage = "Memo group was not found.";
        (void)database.rollback();
        return false;
    }

    if (!database.commit()) {
        repository.errorMessage = database.lastError().text();
        (void)database.rollback();
        return false;
    }

    repository.errorMessage.clear();
    return true;
}

bool MemoRepository::getDefaultGroup(MemoGroup* group) {
    MemoRepository& repository = instance();
    if (group == nullptr) {
        repository.errorMessage = "MemoGroup pointer is null.";
        return false;
    }

    if (!repository.ensureDatabaseReady()) {
        return false;
    }

    QSqlDatabase database = MemoDatabase::database();
    QSqlQuery query(database);
    if (!query.exec(
            "SELECT id, name, sort_order, is_default, created_at, updated_at "
            "FROM memo_group WHERE is_default = 1 "
            "ORDER BY sort_order ASC, id ASC LIMIT 1")) {
        repository.errorMessage = query.lastError().text();
        return false;
    }

    if (!query.next()) {
        repository.errorMessage.clear();
        return false;
    }

    *group = repository.mapGroupFromQuery(query);
    repository.errorMessage.clear();
    return true;
}

QList<MemoGroup> MemoRepository::listGroups() {
    MemoRepository& repository = instance();
    QList<MemoGroup> groups;
    if (!repository.ensureDatabaseReady()) {
        return groups;
    }

    QSqlDatabase database = MemoDatabase::database();
    QSqlQuery query(database);
    if (!query.exec(
            "SELECT id, name, sort_order, is_default, created_at, updated_at "
            "FROM memo_group "
            "ORDER BY is_default DESC, sort_order ASC, id ASC")) {
        repository.errorMessage = query.lastError().text();
        return groups;
    }

    while (query.next()) {
        groups.append(repository.mapGroupFromQuery(query));
    }

    repository.errorMessage.clear();
    return groups;
}

bool MemoRepository::addRecord(const MemoRecord& record, qint64* insertedId) {
    MemoRepository& repository = instance();
    if (!repository.ensureDatabaseReady()) {
        return false;
    }

    QSqlDatabase database = MemoDatabase::database();
    QSqlQuery query(database);
    query.prepare(
        "INSERT INTO memo_record ("
        "group_id, title, content_html, content_plain, status, "
        "plan_due_at, plan_delay_seconds, completed_at, "
        "created_at, updated_at, deleted"
        ") VALUES ("
        ":group_id, :title, :content_html, :content_plain, :status, "
        ":plan_due_at, :plan_delay_seconds, :completed_at, "
        ":created_at, :updated_at, :deleted)"
    );
    query.bindValue(":group_id", record.groupId);
    query.bindValue(":title", record.title.isNull() ? QStringLiteral("未命名标题") : record.title);
    query.bindValue(":content_html", record.contentHtml.isNull() ? QStringLiteral("") : record.contentHtml);
    query.bindValue(":content_plain", record.contentPlain.isNull() ? QStringLiteral("") : record.contentPlain);
    query.bindValue(":status", static_cast<int>(record.status));
    query.bindValue(":plan_due_at", record.planDueAt);
    query.bindValue(":plan_delay_seconds", record.planDelaySeconds);
    query.bindValue(":completed_at", record.completedAt);
    query.bindValue(":created_at", record.createdAt);
    query.bindValue(":updated_at", record.updatedAt);
    query.bindValue(":deleted", record.deleted ? 1 : 0);

    if (!query.exec()) {
        repository.errorMessage = query.lastError().text();
        return false;
    }

    if (insertedId != nullptr) {
        *insertedId = query.lastInsertId().toLongLong();
    }

    repository.errorMessage.clear();
    return true;
}

bool MemoRepository::updateRecord(const MemoRecord& record) {
    MemoRepository& repository = instance();
    if (!repository.ensureDatabaseReady()) {
        return false;
    }

    QSqlDatabase database = MemoDatabase::database();
    QSqlQuery query(database);
    query.prepare(
        "UPDATE memo_record "
        "SET group_id = :group_id, "
        "title = :title, "
        "content_html = :content_html, "
        "content_plain = :content_plain, "
        "status = :status, "
        "plan_due_at = :plan_due_at, "
        "plan_delay_seconds = :plan_delay_seconds, "
        "completed_at = :completed_at, "
        "updated_at = :updated_at, "
        "deleted = :deleted "
        "WHERE id = :id"
    );
    query.bindValue(":id", record.id);
    query.bindValue(":group_id", record.groupId);
    query.bindValue(":title", record.title.isNull() ? QStringLiteral("未命名标题") : record.title);
    query.bindValue(":content_html", record.contentHtml.isNull() ? QStringLiteral("") : record.contentHtml);
    query.bindValue(":content_plain", record.contentPlain.isNull() ? QStringLiteral("") : record.contentPlain);
    query.bindValue(":status", static_cast<int>(record.status));
    query.bindValue(":plan_due_at", record.planDueAt);
    query.bindValue(":plan_delay_seconds", record.planDelaySeconds);
    query.bindValue(":completed_at", record.completedAt);
    query.bindValue(":updated_at", record.updatedAt);
    query.bindValue(":deleted", record.deleted ? 1 : 0);

    if (!query.exec()) {
        repository.errorMessage = query.lastError().text();
        return false;
    }

    repository.errorMessage.clear();
    return query.numRowsAffected() > 0;
}

bool MemoRepository::deleteRecord(qint64 recordId, qint64 deletedAt) {
    MemoRepository& repository = instance();
    if (recordId <= 0 || deletedAt <= 0) {
        repository.errorMessage = "Memo record id or deleted timestamp is invalid.";
        return false;
    }

    if (!repository.ensureDatabaseReady()) {
        return false;
    }

    QSqlDatabase database = MemoDatabase::database();
    if (!database.transaction()) {
        repository.errorMessage = database.lastError().text();
        return false;
    }

    QSqlQuery deleteRecordQuery(database);
    deleteRecordQuery.prepare(
        "UPDATE memo_record SET deleted = 1, updated_at = :updated_at "
        "WHERE id = :id AND deleted = 0");
    deleteRecordQuery.bindValue(":id", recordId);
    deleteRecordQuery.bindValue(":updated_at", deletedAt);
    if (!deleteRecordQuery.exec()) {
        repository.errorMessage = deleteRecordQuery.lastError().text();
        (void)database.rollback();
        return false;
    }

    if (deleteRecordQuery.numRowsAffected() != 1) {
        repository.errorMessage = "Memo record was not found.";
        (void)database.rollback();
        return false;
    }

    QSqlQuery disableReminderQuery(database);
    disableReminderQuery.prepare(
        "UPDATE memo_reminder SET is_enabled = 0, updated_at = :updated_at "
        "WHERE record_id = :record_id");
    disableReminderQuery.bindValue(":record_id", recordId);
    disableReminderQuery.bindValue(":updated_at", deletedAt);
    if (!disableReminderQuery.exec()) {
        repository.errorMessage = disableReminderQuery.lastError().text();
        (void)database.rollback();
        return false;
    }

    if (!database.commit()) {
        repository.errorMessage = database.lastError().text();
        (void)database.rollback();
        return false;
    }

    repository.errorMessage.clear();
    return true;
}

bool MemoRepository::getRecordById(qint64 id, MemoRecord* record) {
    MemoRepository& repository = instance();
    if (record == nullptr) {
        repository.errorMessage = "MemoRecord pointer is null.";
        return false;
    }

    if (!repository.ensureDatabaseReady()) {
        return false;
    }

    QSqlDatabase database = MemoDatabase::database();
    QSqlQuery query(database);
    query.prepare(
        "SELECT id, group_id, title, content_html, content_plain, status, "
        "plan_due_at, plan_delay_seconds, completed_at, created_at, updated_at, deleted "
        "FROM memo_record WHERE id = :id"
    );
    query.bindValue(":id", id);

    if (!query.exec()) {
        repository.errorMessage = query.lastError().text();
        return false;
    }

    if (!query.next()) {
        repository.errorMessage.clear();
        return false;
    }

    *record = repository.mapRecordFromQuery(query);
    repository.errorMessage.clear();
    return true;
}

QList<MemoRecord> MemoRepository::listRecords(bool includeDeleted) {
    MemoRepository& repository = instance();
    QList<MemoRecord> records;
    if (!repository.ensureDatabaseReady()) {
        return records;
    }

    QSqlDatabase database = MemoDatabase::database();
    QString sql =
        "SELECT id, group_id, title, content_html, content_plain, status, "
        "plan_due_at, plan_delay_seconds, completed_at, created_at, updated_at, deleted "
        "FROM memo_record ";
    if (!includeDeleted) {
        sql += "WHERE deleted = 0 ";
    }
    sql += "ORDER BY updated_at DESC, id DESC";

    QSqlQuery query(database);
    if (!query.exec(sql)) {
        repository.errorMessage = query.lastError().text();
        return records;
    }

    while (query.next()) {
        records.append(repository.mapRecordFromQuery(query));
    }

    repository.errorMessage.clear();
    return records;
}

bool MemoRepository::addReminder(const MemoReminder& reminder, qint64* insertedId) {
    MemoRepository& repository = instance();
    if (!repository.ensureDatabaseReady()) {
        return false;
    }

    QSqlDatabase database = MemoDatabase::database();
    QSqlQuery query(database);
    query.prepare(
        "INSERT INTO memo_reminder ("
        "record_id, start_remind_at, due_at, finish_within_seconds, "
        "remind_interval_seconds, repeat_mode, next_remind_at, "
        "last_reminded_at, is_enabled, auto_update_status, urge_repeat_enabled, created_at, updated_at"
        ") VALUES ("
        ":record_id, :start_remind_at, :due_at, :finish_within_seconds, "
        ":remind_interval_seconds, :repeat_mode, :next_remind_at, "
        ":last_reminded_at, :is_enabled, :auto_update_status, :urge_repeat_enabled, :created_at, :updated_at)"
    );
    query.bindValue(":record_id", reminder.recordId);
    query.bindValue(":start_remind_at", reminder.startRemindAt);
    query.bindValue(":due_at", reminder.dueAt);
    query.bindValue(":finish_within_seconds", reminder.finishWithinSeconds);
    query.bindValue(":remind_interval_seconds", reminder.remindIntervalSeconds);
    query.bindValue(":repeat_mode", static_cast<int>(reminder.repeatMode));
    query.bindValue(":next_remind_at", reminder.nextRemindAt);
    query.bindValue(":last_reminded_at", reminder.lastRemindedAt);
    query.bindValue(":is_enabled", reminder.isEnabled ? 1 : 0);
    query.bindValue(":auto_update_status", reminder.autoUpdateStatus ? 1 : 0);
    query.bindValue(":urge_repeat_enabled", reminder.urgeRepeatEnabled ? 1 : 0);
    query.bindValue(":created_at", reminder.createdAt);
    query.bindValue(":updated_at", reminder.updatedAt);

    if (!query.exec()) {
        repository.errorMessage = query.lastError().text();
        return false;
    }

    if (insertedId != nullptr) {
        *insertedId = query.lastInsertId().toLongLong();
    }

    repository.errorMessage.clear();
    return true;
}

bool MemoRepository::updateReminder(const MemoReminder& reminder) {
    MemoRepository& repository = instance();
    if (!repository.ensureDatabaseReady()) {
        return false;
    }

    QSqlDatabase database = MemoDatabase::database();
    QSqlQuery query(database);
    query.prepare(
        "UPDATE memo_reminder "
        "SET start_remind_at = :start_remind_at, "
        "due_at = :due_at, "
        "finish_within_seconds = :finish_within_seconds, "
        "remind_interval_seconds = :remind_interval_seconds, "
        "repeat_mode = :repeat_mode, "
        "next_remind_at = :next_remind_at, "
        "last_reminded_at = :last_reminded_at, "
        "is_enabled = :is_enabled, "
        "auto_update_status = :auto_update_status, "
        "urge_repeat_enabled = :urge_repeat_enabled, "
        "updated_at = :updated_at "
        "WHERE record_id = :record_id"
    );
    query.bindValue(":record_id", reminder.recordId);
    query.bindValue(":start_remind_at", reminder.startRemindAt);
    query.bindValue(":due_at", reminder.dueAt);
    query.bindValue(":finish_within_seconds", reminder.finishWithinSeconds);
    query.bindValue(":remind_interval_seconds", reminder.remindIntervalSeconds);
    query.bindValue(":repeat_mode", static_cast<int>(reminder.repeatMode));
    query.bindValue(":next_remind_at", reminder.nextRemindAt);
    query.bindValue(":last_reminded_at", reminder.lastRemindedAt);
    query.bindValue(":is_enabled", reminder.isEnabled ? 1 : 0);
    query.bindValue(":auto_update_status", reminder.autoUpdateStatus ? 1 : 0);
    query.bindValue(":urge_repeat_enabled", reminder.urgeRepeatEnabled ? 1 : 0);
    query.bindValue(":updated_at", reminder.updatedAt);

    if (!query.exec()) {
        repository.errorMessage = query.lastError().text();
        return false;
    }

    repository.errorMessage.clear();
    return query.numRowsAffected() > 0;
}

bool MemoRepository::getReminderByRecordId(qint64 recordId, MemoReminder* reminder) {
    MemoRepository& repository = instance();
    if (reminder == nullptr) {
        repository.errorMessage = "MemoReminder pointer is null.";
        return false;
    }

    if (!repository.ensureDatabaseReady()) {
        return false;
    }

    QSqlDatabase database = MemoDatabase::database();
    QSqlQuery query(database);
    query.prepare(
        "SELECT id, record_id, start_remind_at, due_at, finish_within_seconds, "
        "remind_interval_seconds, repeat_mode, next_remind_at, "
        "last_reminded_at, is_enabled, auto_update_status, urge_repeat_enabled, created_at, updated_at "
        "FROM memo_reminder WHERE record_id = :record_id"
    );
    query.bindValue(":record_id", recordId);

    if (!query.exec()) {
        repository.errorMessage = query.lastError().text();
        return false;
    }

    if (!query.next()) {
        repository.errorMessage.clear();
        return false;
    }

    *reminder = repository.mapReminderFromQuery(query);
    repository.errorMessage.clear();
    return true;
}

QList<MemoReminder> MemoRepository::listEnabledReminders() {
    MemoRepository& repository = instance();
    QList<MemoReminder> reminders;
    if (!repository.ensureDatabaseReady()) {
        return reminders;
    }

    QSqlDatabase database = MemoDatabase::database();
    QSqlQuery query(database);
    if (!query.exec(
            "SELECT id, record_id, start_remind_at, due_at, finish_within_seconds, "
            "remind_interval_seconds, repeat_mode, next_remind_at, "
            "last_reminded_at, is_enabled, auto_update_status, urge_repeat_enabled, created_at, updated_at "
            "FROM memo_reminder WHERE is_enabled = 1 "
            "ORDER BY next_remind_at ASC, id ASC")) {
        repository.errorMessage = query.lastError().text();
        return reminders;
    }

    while (query.next()) {
        reminders.append(repository.mapReminderFromQuery(query));
    }

    repository.errorMessage.clear();
    return reminders;
}

QString MemoRepository::lastError() {
    return instance().errorMessage;
}

QString MemoRepository::databasePath() {
    return MemoDatabase::databasePath();
}

bool MemoRepository::ensureDatabaseReady() {
    if (!MemoDatabase::initialize()) {
        errorMessage = MemoDatabase::lastError();
        return false;
    }

    errorMessage.clear();
    return true;
}

MemoGroup MemoRepository::mapGroupFromQuery(const QSqlQuery& query) const {
    MemoGroup group;
    group.id = query.value("id").toLongLong();
    group.name = query.value("name").toString();
    group.sortOrder = query.value("sort_order").toInt();
    group.isDefault = query.value("is_default").toInt() != 0;
    group.createdAt = query.value("created_at").toLongLong();
    group.updatedAt = query.value("updated_at").toLongLong();
    return group;
}

MemoRecord MemoRepository::mapRecordFromQuery(const QSqlQuery& query) const {
    MemoRecord record;
    record.id = query.value("id").toLongLong();
    record.groupId = query.value("group_id").toLongLong();
    record.title = query.value("title").toString();
    record.contentHtml = query.value("content_html").toString();
    record.contentPlain = query.value("content_plain").toString();
    record.status = static_cast<MemoStatus>(query.value("status").toInt());
    record.planDueAt = query.value("plan_due_at").toLongLong();
    record.planDelaySeconds = query.value("plan_delay_seconds").toLongLong();
    record.completedAt = query.value("completed_at").toLongLong();
    record.createdAt = query.value("created_at").toLongLong();
    record.updatedAt = query.value("updated_at").toLongLong();
    record.deleted = query.value("deleted").toInt() != 0;
    return record;
}

MemoReminder MemoRepository::mapReminderFromQuery(const QSqlQuery& query) const {
    MemoReminder reminder;
    reminder.id = query.value("id").toLongLong();
    reminder.recordId = query.value("record_id").toLongLong();
    reminder.startRemindAt = query.value("start_remind_at").toLongLong();
    reminder.dueAt = query.value("due_at").toLongLong();
    reminder.finishWithinSeconds = query.value("finish_within_seconds").toLongLong();
    reminder.remindIntervalSeconds = query.value("remind_interval_seconds").toLongLong();
    reminder.repeatMode = static_cast<MemoReminderRepeatMode>(query.value("repeat_mode").toInt());
    reminder.nextRemindAt = query.value("next_remind_at").toLongLong();
    reminder.lastRemindedAt = query.value("last_reminded_at").toLongLong();
    reminder.isEnabled = query.value("is_enabled").toInt() != 0;
    reminder.autoUpdateStatus = query.value("auto_update_status").toInt() != 0;
    reminder.urgeRepeatEnabled = query.value("urge_repeat_enabled").toInt() != 0;
    reminder.createdAt = query.value("created_at").toLongLong();
    reminder.updatedAt = query.value("updated_at").toLongLong();
    return reminder;
}
