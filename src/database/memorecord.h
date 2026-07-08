#pragma once

#include <qglobal.h>
#include <qstring.h>

enum class MemoStatus : int {
    InProgress = 0, // 进行中
    NotStarted = 1, // 未开始
    Completed = 2,  // 已完成
    Planned = 3,    // 计划中
    TimedOut = 4    // 已超时
};

struct MemoRecord {
    qint64 id = 0;
    qint64 groupId = 0;          // 所属分组 ID
    QString title;               // 记录标题
    QString contentHtml;         // 富文本正文，直接保存 QTextDocument/QTextEdit 导出的 HTML
    QString contentPlain;        // 纯文本正文，用于列表摘要和关键字搜索
    MemoStatus status = MemoStatus::NotStarted;
    qint64 planDueAt = 0;        // 计划完成的绝对时间戳，0 表示未设置
    qint64 planDelaySeconds = 0; // 用户设置的“多久后完成”秒数，便于界面回显
    qint64 completedAt = 0;      // 实际完成时间戳
    qint64 createdAt = 0;        // 创建时间戳
    qint64 updatedAt = 0;        // 最后更新时间戳
    bool deleted = false;        // 软删除标记
};
