#pragma once

#include "memotreedelegate.h"
#include "memotreemodel.h"

#include <qobject.h>

class QTreeView;
class QModelIndex;
class QPoint;
struct MemoReminder;
class MemoMenu;

class MemoTreeController : public QObject
{
    Q_OBJECT

public:
    explicit MemoTreeController(QTreeView* treeView, QObject* parent = nullptr);

    void initialize();
    void addGroup();
    void addRecordToDefaultGroup();
    void addRecordToGroup(qint64 groupId);
    void refresh();

private slots:
    void onTreeItemDoubleClicked(const QModelIndex& index);
    void onTreeContextMenuRequested(const QPoint& position);

private:
    void configureTreeView();
    void reloadTree();
    bool renameGroup(qint64 groupId, const QString& groupName);
    bool loadReminderForRecord(qint64 recordId, MemoReminder* reminder, bool* existed) const;
    bool openRecordEditor(MemoRecord* record,
                          MemoReminder* reminder,
                          bool* shouldPersistReminder,
                          bool reminderExisted,
                          bool createMode);
    bool persistReminderForRecord(const MemoReminder& reminder, bool reminderExisted);
    void beginRenameGroup(qint64 groupId);
    void editRecord(qint64 recordId);
    void changeRecordStatus(qint64 recordId, MemoStatus status);
    void moveRecord(qint64 recordId, qint64 targetGroupId);
    void deleteRecord(qint64 recordId);
    void deleteGroup(qint64 groupId);
    void showRepositoryError(const QString& operation) const;
    MemoGroup createGroupDraft() const;
    MemoRecord createRecordDraft(qint64 groupId) const;

private:
    QTreeView* treeView = nullptr;
    MemoTreeModel* model = nullptr;
    MemoTreeDelegate* delegate = nullptr;
    MemoMenu* contextMenu = nullptr;
    qint64 pendingEditGroupId = 0;
    qint64 pendingFocusRecordId = 0;
};
