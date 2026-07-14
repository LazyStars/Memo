#include "memoremindercontroller.h"

#include "database/memogroup.h"
#include "database/memorecord.h"
#include "database/memoreminder.h"
#include "database/memorepository.h"
#include "tools/tray/mysystemtrayicon.h"

#include <qdatetime.h>
#include <qmessagebox.h>
#include <qpushbutton.h>
#include <qtimer.h>
#include <qwidget.h>

MemoReminderController::MemoReminderController(MySystemTrayIcon* trayIconValue,
                                               QWidget* parentWidgetValue,
                                               QObject* parent)
    : QObject(parent), trayIcon(trayIconValue), parentWidget(parentWidgetValue) {
}

void MemoReminderController::processDueReminders(qint64 currentTimestamp) {
    groupNames.clear();
    for (const MemoGroup& group : MemoRepository::listGroups()) {
        groupNames.insert(group.id, group.name);
    }

    const QList<MemoReminder> reminders = MemoRepository::listEnabledReminders();
    for (const MemoReminder& reminder : reminders) {
        processReminder(reminder, currentTimestamp);
    }
}

void MemoReminderController::processReminder(MemoReminder reminder, qint64 currentTimestamp) {
    MemoRecord record;
    if (!MemoRepository::getRecordById(reminder.recordId, &record) || record.deleted
        || record.status == MemoStatus::Completed) {
        if (disableReminder(&reminder, currentTimestamp)) {
            Q_EMIT reminderDataChanged();
        }
        return;
    }

    const bool hasDueReminder = reminder.dueAt > 0;
    if (hasDueReminder && currentTimestamp >= reminder.dueAt) {
        if (reminder.autoUpdateStatus) {
            if (!completeRecord(record.id, currentTimestamp)) {
                return;
            }

            showDueNotification(record, groupNameForRecord(record.groupId));
            return;
        }

        const bool isFirstDueReminder = reminder.lastRemindedAt < reminder.dueAt;
        const bool isRepeatedDueReminder = reminder.urgeRepeatEnabled
                                           && reminder.remindIntervalSeconds > 0
                                           && currentTimestamp >= reminder.nextRemindAt;
        if (!isFirstDueReminder && !isRepeatedDueReminder) {
            return;
        }

        reminder.lastRemindedAt = currentTimestamp;
        reminder.nextRemindAt = reminder.urgeRepeatEnabled
                                    ? currentTimestamp + reminder.remindIntervalSeconds
                                    : 0;
        reminder.isEnabled = reminder.urgeRepeatEnabled;
        if (!persistReminder(&reminder, currentTimestamp)) {
            return;
        }

        showDueReminder(record, groupNameForRecord(record.groupId));
        Q_EMIT reminderDataChanged();
        return;
    }

    if (reminder.startRemindAt <= 0 || currentTimestamp < reminder.startRemindAt) {
        return;
    }

    const bool isFirstStartReminder = reminder.lastRemindedAt < reminder.startRemindAt;
    const bool canRepeatStartReminder = reminder.repeatMode == MemoReminderRepeatMode::Repeat
                                        && record.status != MemoStatus::InProgress
                                        && currentTimestamp >= reminder.nextRemindAt;
    if (!isFirstStartReminder && !canRepeatStartReminder) {
        return;
    }

    if (isFirstStartReminder && reminder.autoUpdateStatus) {
        record.status = MemoStatus::InProgress;
        record.updatedAt = currentTimestamp;
        record.completedAt = 0;
        if (!MemoRepository::updateRecord(record)) {
            return;
        }
    }

    showStartNotification(record, groupNameForRecord(record.groupId));

    reminder.lastRemindedAt = currentTimestamp;
    if (reminder.autoUpdateStatus && hasDueReminder) {
        reminder.nextRemindAt = reminder.dueAt;
    } else if (reminder.repeatMode == MemoReminderRepeatMode::Repeat
        && record.status != MemoStatus::InProgress) {
        reminder.nextRemindAt = currentTimestamp + reminder.remindIntervalSeconds;
    } else if (hasDueReminder) {
        reminder.nextRemindAt = reminder.dueAt;
    } else {
        reminder.isEnabled = false;
        reminder.nextRemindAt = 0;
    }

    if (persistReminder(&reminder, currentTimestamp)) {
        Q_EMIT reminderDataChanged();
    }
}

bool MemoReminderController::persistReminder(MemoReminder* reminder, qint64 currentTimestamp) {
    if (reminder == nullptr) {
        return false;
    }

    reminder->updatedAt = currentTimestamp;
    return MemoRepository::updateReminder(*reminder);
}

bool MemoReminderController::disableReminder(MemoReminder* reminder, qint64 currentTimestamp) {
    if (reminder == nullptr || !reminder->isEnabled) {
        return false;
    }

    reminder->isEnabled = false;
    reminder->nextRemindAt = 0;
    return persistReminder(reminder, currentTimestamp);
}

void MemoReminderController::showStartNotification(const MemoRecord& record, const QString& groupName) {
    if (trayIcon == nullptr) {
        return;
    }

    trayIcon->showMessage(QStringLiteral("定时提醒"),
                          QStringLiteral("%1 - %2 开始了").arg(groupName, record.title),
                          QSystemTrayIcon::Information,
                          5000);
}

void MemoReminderController::showDueReminder(const MemoRecord& record, const QString& groupName) {
    if (parentWidget == nullptr || visibleDueRecordIds.contains(record.id)) {
        return;
    }

    visibleDueRecordIds.insert(record.id);
    auto* messageBox = new QMessageBox(QMessageBox::Information,
                                       QStringLiteral("督促提醒"),
                                       QStringLiteral("%1 - %2 的预计完成时间已到达。")
                                           .arg(groupName, record.title),
                                       QMessageBox::NoButton,
                                       parentWidget);
    messageBox->setWindowModality(Qt::NonModal);
    messageBox->setAttribute(Qt::WA_DeleteOnClose);
    QAbstractButton* completeButton = messageBox->addButton(QStringLiteral("标记为已完成"),
                                                             QMessageBox::AcceptRole);
    messageBox->addButton(QMessageBox::Close);
    connect(messageBox, &QMessageBox::buttonClicked, this,
            [this, recordId = record.id, completeButton](QAbstractButton* button) {
        if (button == completeButton) {
            (void)completeRecord(recordId, QDateTime::currentSecsSinceEpoch());
        }
    });
    connect(messageBox, &QObject::destroyed, this, [this, recordId = record.id] {
        visibleDueRecordIds.remove(recordId);
    });
    QTimer::singleShot(5000, messageBox, [messageBox] {
        if (messageBox->isVisible()) {
            messageBox->close();
        }
    });
    messageBox->open();
}

void MemoReminderController::showDueNotification(const MemoRecord& record, const QString& groupName) {
    if (trayIcon == nullptr) {
        return;
    }

    trayIcon->showMessage(QStringLiteral("督促提醒"),
                          QStringLiteral("%1 - %2 的预计完成时间已到达。")
                              .arg(groupName, record.title),
                          QSystemTrayIcon::Information,
                          5000);
}

bool MemoReminderController::completeRecord(qint64 recordId, qint64 currentTimestamp) {
    MemoRecord record;
    if (!MemoRepository::getRecordById(recordId, &record) || record.deleted) {
        return false;
    }

    record.status = MemoStatus::Completed;
    record.completedAt = currentTimestamp;
    record.updatedAt = currentTimestamp;
    if (!MemoRepository::updateRecord(record)) {
        return false;
    }

    MemoReminder reminder;
    if (MemoRepository::getReminderByRecordId(recordId, &reminder)) {
        if (reminder.isEnabled && !disableReminder(&reminder, currentTimestamp)) {
            return false;
        }
    }

    Q_EMIT reminderDataChanged();
    return true;
}

QString MemoReminderController::groupNameForRecord(qint64 groupId) const {
    return groupNames.value(groupId, QStringLiteral("未分组"));
}
