#pragma once

#include "database/memogroup.h"
#include "database/memorecord.h"

#include <qlist.h>
#include <qmenu.h>
#include <qpoint.h>

class QPaintEvent;

class MemoMenu final : public QMenu
{
    Q_OBJECT

public:
    explicit MemoMenu(QWidget* parent = nullptr);

    void showForGroup(const MemoGroup& group, bool containsRecords, const QPoint& globalPosition);
    void showForRecord(const MemoRecord& record,
                       const QList<MemoGroup>& availableGroups,
                       const QPoint& globalPosition);

signals:
    void requestAddRecord(qint64 groupId);
    void requestRenameGroup(qint64 groupId);
    void requestDeleteGroup(qint64 groupId);
    void requestEditRecord(qint64 recordId);
    void requestChangeRecordStatus(qint64 recordId, MemoStatus status);
    void requestMoveRecord(qint64 recordId, qint64 targetGroupId);
    void requestDeleteRecord(qint64 recordId);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void configureStyle();
    void resetActions();
    QString statusText(MemoStatus status) const;
};
