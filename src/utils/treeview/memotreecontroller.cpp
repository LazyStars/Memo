#include "memotreecontroller.h"

#include "database/memorepository.h"

#include <qabstractitemview.h>
#include <qdatetime.h>
#include <qdebug.h>
#include <qheaderview.h>
#include <qtreeview.h>
#include <qwidget.h>

MemoTreeController::MemoTreeController(QTreeView* treeViewWidget, QObject* parent)
    : QObject(parent), treeView(treeViewWidget) {
    model = new MemoTreeModel(this);
    delegate = new MemoTreeDelegate(this);
}

void MemoTreeController::initialize() {
    configureTreeView();
    connect(delegate, &MemoTreeDelegate::requestAddRecord, this, &MemoTreeController::addRecordToGroup);
    connect(delegate, &MemoTreeDelegate::requestRenameGroup, this, [this](qint64 groupId, const QString& groupName) {
        if (!renameGroup(groupId, groupName)) {
            qWarning() << "rename memo group failed, group id:" << groupId
                       << "error:" << MemoRepository::lastError();
        }
    });
    connect(treeView, &QTreeView::doubleClicked, this, &MemoTreeController::onTreeItemDoubleClicked);
    reloadTree();
}

void MemoTreeController::addGroup() {
    MemoGroup group = createGroupDraft();
    qint64 insertedId = 0;
    if (!MemoRepository::addGroup(group, &insertedId)) {
        qWarning() << "add memo group failed:" << MemoRepository::lastError();
        return;
    }

    pendingEditGroupId = insertedId;
    reloadTree();
}

void MemoTreeController::addRecordToDefaultGroup() {
    MemoGroup defaultGroup;
    if (!MemoRepository::getDefaultGroup(&defaultGroup)) {
        qWarning() << "get default group failed:" << MemoRepository::lastError();
        return;
    }

    addRecordToGroup(defaultGroup.id);
}

void MemoTreeController::addRecordToGroup(qint64 groupId) {
    MemoRecord record = createRecordDraft(groupId);
    if (!MemoRepository::addRecord(record, nullptr)) {
        qWarning() << "add memo record failed, group id:" << groupId
                   << "error:" << MemoRepository::lastError();
        return;
    }

    reloadTree();
}

void MemoTreeController::onTreeItemDoubleClicked(const QModelIndex& index) {
    MemoTreeNode* node = model->nodeFromIndex(index);
    if (node == nullptr || node->type() != MemoTreeNodeType::Group) {
        return;
    }

    treeView->edit(index);
}

void MemoTreeController::configureTreeView() {
    delegate->setTreeView(treeView);
    treeView->setModel(model);
    treeView->setItemDelegate(delegate);
    treeView->setHeaderHidden(true);
    treeView->setRootIsDecorated(false);
    treeView->setItemsExpandable(true);
    treeView->setExpandsOnDoubleClick(false);
    treeView->setIndentation(12);
    treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    treeView->setSelectionMode(QAbstractItemView::SingleSelection);
    treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
    treeView->setUniformRowHeights(false);
    treeView->setMouseTracking(true);
    treeView->viewport()->setMouseTracking(true);
    treeView->header()->setStretchLastSection(true);
}

void MemoTreeController::reloadTree() {
    const QList<MemoGroup> groups = MemoRepository::listGroups();
    if (groups.isEmpty()) {
        qWarning() << "memo tree reload found no groups.";
    }

    const QList<MemoRecord> records = MemoRepository::listRecords();
    model->rebuild(groups, records);
    treeView->expandAll();

    if (pendingEditGroupId > 0) {
        const QModelIndex groupIndex = model->groupIndex(pendingEditGroupId);
        if (groupIndex.isValid()) {
            treeView->scrollTo(groupIndex);
            treeView->edit(groupIndex);
        }
        pendingEditGroupId = 0;
    }
}

bool MemoTreeController::renameGroup(qint64 groupId, const QString& groupName) {
    const QString trimmedGroupName = groupName.trimmed();
    if (trimmedGroupName.isEmpty()) {
        return false;
    }

    const QList<MemoGroup> groups = MemoRepository::listGroups();
    for (const MemoGroup& group : groups) {
        if (group.id != groupId) {
            continue;
        }

        if (group.name == trimmedGroupName) {
            return true;
        }

        MemoGroup updatedGroup = group;
        updatedGroup.name = trimmedGroupName;
        updatedGroup.updatedAt = QDateTime::currentSecsSinceEpoch();
        if (!MemoRepository::updateGroup(updatedGroup)) {
            return false;
        }

        reloadTree();
        return true;
    }

    return false;
}

MemoGroup MemoTreeController::createGroupDraft() const {
    MemoGroup group;
    const QList<MemoGroup> groups = MemoRepository::listGroups();

    int maxSortOrder = 0;
    for (const MemoGroup& existingGroup : groups) {
        if (existingGroup.sortOrder > maxSortOrder) {
            maxSortOrder = existingGroup.sortOrder;
        }
    }

    const qint64 now = QDateTime::currentSecsSinceEpoch();
    group.name = QStringLiteral("新建分组");
    group.sortOrder = maxSortOrder + 1;
    group.isDefault = false;
    group.createdAt = now;
    group.updatedAt = now;
    return group;
}

MemoRecord MemoTreeController::createRecordDraft(qint64 groupId) const {
    MemoRecord record;
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    record.groupId = groupId;
    record.title = QStringLiteral("未命名标题");
    record.contentHtml = QStringLiteral("");
    record.contentPlain = QStringLiteral("");
    record.status = MemoStatus::NotStarted;
    record.planDueAt = 0;
    record.planDelaySeconds = 0;
    record.completedAt = 0;
    record.createdAt = now;
    record.updatedAt = now;
    record.deleted = false;
    return record;
}
