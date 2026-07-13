#include "memomenu.h"

#include <qaction.h>
#include <qicon.h>
#include <qpainter.h>

MemoMenu::MemoMenu(QWidget* parent) : QMenu(parent) {
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    configureStyle();
    setStyleSheet(styleSheet() + QStringLiteral("QMenu { background: transparent; border: none; }"));
}

void MemoMenu::showForGroup(const MemoGroup& group, bool containsRecords, const QPoint& globalPosition) {
    resetActions();
    if (group.id <= 0) {
        return;
    }

    QAction* addRecordAction = addAction(QIcon(QStringLiteral(":/btnicon/res/btnicon/add.png")),
                                         QStringLiteral("新建记录"));
    connect(addRecordAction, &QAction::triggered, this, [this, groupId = group.id] {
        Q_EMIT requestAddRecord(groupId);
    });

    addSeparator();
    QAction* renameAction = addAction(QStringLiteral("重命名"));
    connect(renameAction, &QAction::triggered, this, [this, groupId = group.id] {
        Q_EMIT requestRenameGroup(groupId);
    });

    QAction* deleteAction = addAction(QStringLiteral("删除分组"));
    const bool canDelete = !group.isDefault && !containsRecords;
    deleteAction->setEnabled(canDelete);
    if (group.isDefault) {
        deleteAction->setToolTip(QStringLiteral("默认分组不能删除"));
    } else if (containsRecords) {
        deleteAction->setToolTip(QStringLiteral("请先移动或删除分组中的记录"));
    }
    connect(deleteAction, &QAction::triggered, this, [this, groupId = group.id] {
        Q_EMIT requestDeleteGroup(groupId);
    });

    popup(globalPosition);
}

void MemoMenu::showForRecord(const MemoRecord& record,
                             const QList<MemoGroup>& availableGroups,
                             const QPoint& globalPosition) {
    resetActions();
    if (record.id <= 0 || record.deleted) {
        return;
    }

    QAction* editAction = addAction(QIcon(QStringLiteral(":/btnicon/res/btnicon/edit.png")),
                                    QStringLiteral("编辑记录"));
    connect(editAction, &QAction::triggered, this, [this, recordId = record.id] {
        Q_EMIT requestEditRecord(recordId);
    });

    auto* statusMenu = new MemoMenu(this);
    statusMenu->setTitle(QStringLiteral("改变状态"));
    addMenu(statusMenu);
    const QList<MemoStatus> statuses = {
        MemoStatus::InProgress,
        MemoStatus::NotStarted,
        MemoStatus::Completed,
        MemoStatus::Planned,
        MemoStatus::TimedOut
    };
    for (const MemoStatus status : statuses) {
        QAction* statusAction = statusMenu->addAction(statusText(status));
        statusAction->setCheckable(true);
        statusAction->setChecked(record.status == status);
        statusAction->setEnabled(record.status != status);
        connect(statusAction, &QAction::triggered, this, [this, recordId = record.id, status] {
            Q_EMIT requestChangeRecordStatus(recordId, status);
        });
    }

    auto* moveMenu = new MemoMenu(this);
    moveMenu->setTitle(QStringLiteral("移动到"));
    addMenu(moveMenu);
    bool hasMoveTarget = false;
    for (const MemoGroup& group : availableGroups) {
        if (group.id <= 0 || group.id == record.groupId) {
            continue;
        }

        hasMoveTarget = true;
        QAction* moveAction = moveMenu->addAction(group.name);
        connect(moveAction, &QAction::triggered, this, [this, recordId = record.id, groupId = group.id] {
            Q_EMIT requestMoveRecord(recordId, groupId);
        });
    }
    moveMenu->setEnabled(hasMoveTarget);

    addSeparator();
    QAction* deleteAction = addAction(QStringLiteral("删除记录"));
    connect(deleteAction, &QAction::triggered, this, [this, recordId = record.id] {
        Q_EMIT requestDeleteRecord(recordId);
    });

    popup(globalPosition);
}

void MemoMenu::configureStyle() {
    setStyleSheet(QStringLiteral(
        "QMenu { background-color: rgba(48, 54, 66, 245); color: rgb(236, 239, 244); "
        "border: 1px solid rgba(170, 180, 200, 110); border-radius: 6px; padding: 5px; }"
        "QMenu::item { min-width: 136px; padding: 7px 26px 7px 12px; border-radius: 4px; }"
        "QMenu::item:selected { background-color: rgba(92, 122, 170, 150); }"
        "QMenu::item:disabled { color: rgba(196, 202, 212, 110); }"
        "QMenu::separator { height: 1px; background: rgba(255, 255, 255, 32); margin: 5px 8px; }"
        "QMenu::right-arrow { width: 8px; height: 8px; }"));
}

void MemoMenu::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    const QRect backgroundRect = rect().adjusted(0, 0, -1, -1);
    painter.setPen(QPen(QColor(170, 180, 200, 110), 1));
    painter.setBrush(QColor(48, 54, 66, 245));
    painter.drawRoundedRect(backgroundRect, 6, 6);
    painter.end();
    QMenu::paintEvent(event);
}

void MemoMenu::resetActions() {
    clear();
}

QString MemoMenu::statusText(MemoStatus status) const {
    switch (status) {
    case MemoStatus::InProgress:
        return QStringLiteral("进行中");
    case MemoStatus::Completed:
        return QStringLiteral("已完成");
    case MemoStatus::Planned:
        return QStringLiteral("计划中");
    case MemoStatus::TimedOut:
        return QStringLiteral("已超时");
    case MemoStatus::NotStarted:
    default:
        return QStringLiteral("未开始");
    }
}
