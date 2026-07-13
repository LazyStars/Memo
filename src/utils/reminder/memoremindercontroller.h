#pragma once

#include <qobject.h>
#include <qhash.h>
#include <qstring.h>
#include <qset.h>

class MySystemTrayIcon;
class QWidget;
struct MemoReminder;
struct MemoRecord;

class MemoReminderController : public QObject
{
    Q_OBJECT

public:
    explicit MemoReminderController(MySystemTrayIcon* trayIcon,
                                    QWidget* parentWidget,
                                    QObject* parent = nullptr);

    void processDueReminders(qint64 currentTimestamp);

signals:
    void reminderDataChanged();

private:
    void processReminder(MemoReminder reminder, qint64 currentTimestamp);
    bool persistReminder(MemoReminder* reminder, qint64 currentTimestamp);
    bool disableReminder(MemoReminder* reminder, qint64 currentTimestamp);
    void showStartNotification(const MemoRecord& record, const QString& groupName);
    void showDueReminder(const MemoRecord& record, const QString& groupName);
    void completeRecord(qint64 recordId);
    QString groupNameForRecord(qint64 groupId) const;

private:
    MySystemTrayIcon* trayIcon = nullptr;
    QWidget* parentWidget = nullptr;
    QHash<qint64, QString> groupNames;
    QSet<qint64> visibleDueRecordIds;
};
