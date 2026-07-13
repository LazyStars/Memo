#pragma once

#include <qglobal.h>

enum class MemoReminderRepeatMode : int {
    Once = 0,   // 仅提醒一次
    Repeat = 1  // 按设定间隔重复提醒
};

struct MemoReminder {
    qint64 id = 0;
    qint64 recordId = 0;               // 对应的备忘录记录 ID
    qint64 startRemindAt = 0;          // 提醒开始工作的时间戳
    qint64 dueAt = 0;                  // 计划完成的绝对时间戳
    qint64 finishWithinSeconds = 0;    // 从开始到完成的计划用时秒数
    qint64 remindIntervalSeconds = 0;  // 每次提醒的间隔秒数，0 表示不重复
    MemoReminderRepeatMode repeatMode = MemoReminderRepeatMode::Once;
    qint64 nextRemindAt = 0;           // 下次应触发提醒的时间戳
    qint64 lastRemindedAt = 0;         // 最近一次实际提醒时间戳
    bool isEnabled = true;             // Whether this reminder is active.
    bool autoUpdateStatus = false;     // Update status when the reminder starts.
    bool urgeRepeatEnabled = false;    // Repeat overdue prompts when enabled.
    qint64 createdAt = 0;              // 创建时间戳
    qint64 updatedAt = 0;              // 最后更新时间戳
};
