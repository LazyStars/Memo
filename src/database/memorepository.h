#pragma once

#include "memodatabase.h"
#include "memogroup.h"
#include "memorecord.h"
#include "memoreminder.h"

#include <qlist.h>
#include <qstring.h>

class QSqlQuery;

class MemoRepository
{
public:
    static bool addGroup(const MemoGroup& group, qint64* insertedId = nullptr);
    static bool updateGroup(const MemoGroup& group);
    static bool getDefaultGroup(MemoGroup* group);
    static QList<MemoGroup> listGroups();

    static bool addRecord(const MemoRecord& record, qint64* insertedId = nullptr);
    static bool updateRecord(const MemoRecord& record);
    static bool getRecordById(qint64 id, MemoRecord* record);
    static QList<MemoRecord> listRecords(bool includeDeleted = false);

    static bool addReminder(const MemoReminder& reminder, qint64* insertedId = nullptr);
    static bool updateReminder(const MemoReminder& reminder);
    static bool getReminderByRecordId(qint64 recordId, MemoReminder* reminder);
    static QList<MemoReminder> listEnabledReminders();

    static QString lastError();
    static QString databasePath();

private:
    MemoRepository() = default;

    static MemoRepository& instance();
    MemoGroup mapGroupFromQuery(const QSqlQuery& query) const;
    bool ensureDatabaseReady();
    MemoRecord mapRecordFromQuery(const QSqlQuery& query) const;
    MemoReminder mapReminderFromQuery(const QSqlQuery& query) const;

private:
    QString errorMessage; // 最近一次仓储操作错误信息
};
