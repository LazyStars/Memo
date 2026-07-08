#pragma once

#include "memotreedelegate.h"
#include "memotreemodel.h"

#include <qobject.h>

class QTreeView;
class QModelIndex;

class MemoTreeController : public QObject
{
    Q_OBJECT

public:
    explicit MemoTreeController(QTreeView* treeView, QObject* parent = nullptr);

    void initialize();
    void addGroup();
    void addRecordToDefaultGroup();
    void addRecordToGroup(qint64 groupId);

private slots:
    void onTreeItemDoubleClicked(const QModelIndex& index);

private:
    void configureTreeView();
    void reloadTree();
    bool renameGroup(qint64 groupId, const QString& groupName);
    MemoGroup createGroupDraft() const;
    MemoRecord createRecordDraft(qint64 groupId) const;

private:
    QTreeView* treeView = nullptr;
    MemoTreeModel* model = nullptr;
    MemoTreeDelegate* delegate = nullptr;
    qint64 pendingEditGroupId = 0;
};
