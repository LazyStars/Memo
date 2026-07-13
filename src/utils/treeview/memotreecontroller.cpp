#include "memotreecontroller.h"

#include "database/memorepository.h"
#include "dialogs/edittextdialog.h"
#include "tools/scrollbar/silkyscrollbar.h"
#include "utils/menu/memomenu.h"

#include <qabstractitemview.h>
#include <qdatetime.h>
#include <qdebug.h>
#include <qheaderview.h>
#include <qmessagebox.h>
#include <qtreeview.h>
#include <qwidget.h>

MemoTreeController::MemoTreeController(QTreeView* treeViewWidget, QObject* parent)
    : QObject(parent), treeView(treeViewWidget) {
    model = new MemoTreeModel(this);
    delegate = new MemoTreeDelegate(this);
    contextMenu = new MemoMenu(treeView);
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
    connect(treeView, &QTreeView::customContextMenuRequested,
            this, &MemoTreeController::onTreeContextMenuRequested);
    connect(contextMenu, &MemoMenu::requestAddRecord, this, &MemoTreeController::addRecordToGroup);
    connect(contextMenu, &MemoMenu::requestRenameGroup, this, &MemoTreeController::beginRenameGroup);
    connect(contextMenu, &MemoMenu::requestDeleteGroup, this, &MemoTreeController::deleteGroup);
    connect(contextMenu, &MemoMenu::requestEditRecord, this, &MemoTreeController::editRecord);
    connect(contextMenu, &MemoMenu::requestChangeRecordStatus,
            this, &MemoTreeController::changeRecordStatus);
    connect(contextMenu, &MemoMenu::requestMoveRecord, this, &MemoTreeController::moveRecord);
    connect(contextMenu, &MemoMenu::requestDeleteRecord, this, &MemoTreeController::deleteRecord);
    reloadTree();
}

void MemoTreeController::refresh() {
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
    MemoReminder reminder;
    bool shouldPersistReminder = false;
    if (!openRecordEditor(&record, &reminder, &shouldPersistReminder, false, true)) {
        return;
    }

    qint64 insertedId = 0;
    if (!MemoRepository::addRecord(record, &insertedId)) {
        qWarning() << "add memo record failed, group id:" << groupId
                   << "error:" << MemoRepository::lastError();
        return;
    }

    record.id = insertedId;
    if (shouldPersistReminder) {
        reminder.recordId = insertedId;
        if (!persistReminderForRecord(reminder, false)) {
            qWarning() << "persist memo reminder failed, record id:" << insertedId
                       << "error:" << MemoRepository::lastError();
        }
    }

    pendingFocusRecordId = insertedId;
    reloadTree();
}

void MemoTreeController::onTreeItemDoubleClicked(const QModelIndex& index) {
    MemoTreeNode* node = model->nodeFromIndex(index);
    if (node == nullptr) {
        return;
    }

    if (node->type() == MemoTreeNodeType::Group) {
        treeView->edit(index);
        return;
    }

    if (node->type() != MemoTreeNodeType::Record) {
        return;
    }

    editRecord(node->recordData().id);
}

void MemoTreeController::onTreeContextMenuRequested(const QPoint& position) {
    const QModelIndex index = treeView->indexAt(position);
    if (!index.isValid()) {
        return;
    }

    MemoTreeNode* node = model->nodeFromIndex(index);
    if (node == nullptr || contextMenu == nullptr) {
        return;
    }

    treeView->setCurrentIndex(index);
    const QPoint globalPosition = treeView->viewport()->mapToGlobal(position);
    if (node->type() == MemoTreeNodeType::Group) {
        contextMenu->showForGroup(node->groupData(), node->childCount() > 0, globalPosition);
        return;
    }

    if (node->type() == MemoTreeNodeType::Record) {
        contextMenu->showForRecord(node->recordData(), MemoRepository::listGroups(), globalPosition);
    }
}

void MemoTreeController::beginRenameGroup(qint64 groupId) {
    if (groupId <= 0) {
        return;
    }

    const QModelIndex groupIndex = model->groupIndex(groupId);
    if (groupIndex.isValid()) {
        treeView->setCurrentIndex(groupIndex);
        treeView->edit(groupIndex);
    }
}

void MemoTreeController::editRecord(qint64 recordId) {
    MemoRecord record;
    if (recordId <= 0 || !MemoRepository::getRecordById(recordId, &record) || record.deleted) {
        showRepositoryError(QStringLiteral("读取记录失败"));
        return;
    }

    MemoReminder reminder;
    bool reminderExisted = false;
    if (!loadReminderForRecord(record.id, &reminder, &reminderExisted)) {
        showRepositoryError(QStringLiteral("读取记录提醒失败"));
        return;
    }

    bool shouldPersistReminder = false;
    if (!openRecordEditor(&record, &reminder, &shouldPersistReminder, reminderExisted, false)) {
        return;
    }

    if (!MemoRepository::updateRecord(record)) {
        showRepositoryError(QStringLiteral("保存记录失败"));
        return;
    }

    if (shouldPersistReminder && !persistReminderForRecord(reminder, reminderExisted)) {
        showRepositoryError(QStringLiteral("保存记录提醒失败"));
    }

    pendingFocusRecordId = record.id;
    reloadTree();
}

void MemoTreeController::changeRecordStatus(qint64 recordId, MemoStatus status) {
    MemoRecord record;
    if (recordId <= 0 || !MemoRepository::getRecordById(recordId, &record) || record.deleted) {
        showRepositoryError(QStringLiteral("读取记录失败"));
        return;
    }

    if (record.status == status) {
        return;
    }

    record.status = status;
    record.updatedAt = QDateTime::currentSecsSinceEpoch();
    record.completedAt = status == MemoStatus::Completed ? record.updatedAt : 0;
    if (!MemoRepository::updateRecord(record)) {
        showRepositoryError(QStringLiteral("更新记录状态失败"));
        return;
    }

    pendingFocusRecordId = record.id;
    reloadTree();
}

void MemoTreeController::moveRecord(qint64 recordId, qint64 targetGroupId) {
    if (recordId <= 0 || targetGroupId <= 0) {
        return;
    }

    MemoRecord record;
    if (!MemoRepository::getRecordById(recordId, &record) || record.deleted) {
        showRepositoryError(QStringLiteral("读取记录失败"));
        return;
    }

    if (record.groupId == targetGroupId) {
        return;
    }

    bool targetGroupExists = false;
    for (const MemoGroup& group : MemoRepository::listGroups()) {
        if (group.id == targetGroupId) {
            targetGroupExists = true;
            break;
        }
    }
    if (!targetGroupExists) {
        QMessageBox::warning(treeView, QStringLiteral("移动记录失败"), QStringLiteral("目标分组不存在。"));
        return;
    }

    record.groupId = targetGroupId;
    record.updatedAt = QDateTime::currentSecsSinceEpoch();
    if (!MemoRepository::updateRecord(record)) {
        showRepositoryError(QStringLiteral("移动记录失败"));
        return;
    }

    pendingFocusRecordId = record.id;
    reloadTree();
}

void MemoTreeController::deleteRecord(qint64 recordId) {
    if (recordId <= 0) {
        return;
    }

    const QMessageBox::StandardButton button = QMessageBox::question(
        treeView,
        QStringLiteral("删除记录"),
        QStringLiteral("确定要删除这条记录吗？"),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);
    if (button != QMessageBox::Yes) {
        return;
    }

    if (!MemoRepository::deleteRecord(recordId, QDateTime::currentSecsSinceEpoch())) {
        showRepositoryError(QStringLiteral("删除记录失败"));
        return;
    }

    reloadTree();
}

void MemoTreeController::deleteGroup(qint64 groupId) {
    if (groupId <= 0) {
        return;
    }

    MemoGroup groupToDelete;
    bool groupExists = false;
    for (const MemoGroup& group : MemoRepository::listGroups()) {
        if (group.id == groupId) {
            groupToDelete = group;
            groupExists = true;
            break;
        }
    }
    if (!groupExists) {
        QMessageBox::warning(treeView, QStringLiteral("删除分组失败"), QStringLiteral("分组不存在。"));
        return;
    }

    if (groupToDelete.isDefault) {
        QMessageBox::information(treeView, QStringLiteral("删除分组"), QStringLiteral("默认分组不能删除。"));
        return;
    }

    for (const MemoRecord& record : MemoRepository::listRecords()) {
        if (record.groupId == groupId) {
            QMessageBox::information(treeView,
                                     QStringLiteral("删除分组"),
                                     QStringLiteral("请先移动或删除分组中的记录。"));
            return;
        }
    }

    const QMessageBox::StandardButton button = QMessageBox::question(
        treeView,
        QStringLiteral("删除分组"),
        QStringLiteral("确定要删除空分组“%1”吗？").arg(groupToDelete.name),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);
    if (button != QMessageBox::Yes) {
        return;
    }

    if (!MemoRepository::deleteGroup(groupId)) {
        showRepositoryError(QStringLiteral("删除分组失败"));
        return;
    }

    reloadTree();
}

void MemoTreeController::showRepositoryError(const QString& operation) const {
    const QString error = MemoRepository::lastError();
    qWarning() << operation << error;
    QMessageBox::warning(treeView,
                         operation,
                         error.isEmpty() ? QStringLiteral("目标已不存在或不可用。") : error);
}

void MemoTreeController::configureTreeView() {
    auto* verticalScrollBar = new SilkyScrollBar(Qt::Vertical, treeView);
    SilkyScrollBar::ScrollOptions scrollOptions;
    scrollOptions.wheelStep = 92;               // 标准滚轮每格滚动像素，调大滚动更快
    scrollOptions.pixelDeltaFactor = 1.0;       // 触控板滚动缩放，调大更跟手
    scrollOptions.animationDurationMs = 180;    // 动画时长，调大更柔和
    scrollOptions.minimumHandleLength = 40;     // 滑块最小高度，避免内容较多时过细
    scrollOptions.thickness = 10;               // 滚动条厚度
    scrollOptions.margin = 4;                   // 滚动条与 tree view 内容边缘留白
    scrollOptions.radius = 5;                   // 轨道与滑块圆角
    verticalScrollBar->setScrollOptions(scrollOptions);

    delegate->setTreeView(treeView);
    treeView->setVerticalScrollBar(verticalScrollBar);
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
    treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    treeView->header()->setStretchLastSection(true);
    verticalScrollBar->attachTo(treeView);
}

void MemoTreeController::reloadTree() {
    const QList<MemoGroup> groups = MemoRepository::listGroups();
    if (groups.isEmpty()) {
        qWarning() << "memo tree reload found no groups.";
    }

    const QList<MemoRecord> records = MemoRepository::listRecords();
    model->rebuild(groups, records);
    treeView->expandAll();

    if (pendingFocusRecordId > 0) {
        const QModelIndex recordIndex = model->recordIndex(pendingFocusRecordId);
        if (recordIndex.isValid()) {
            treeView->setCurrentIndex(recordIndex);
            treeView->scrollTo(recordIndex, QAbstractItemView::PositionAtCenter);
        }
        pendingFocusRecordId = 0;
    }

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

bool MemoTreeController::loadReminderForRecord(qint64 recordId, MemoReminder* reminder, bool* existed) const {
    if (reminder == nullptr || existed == nullptr) {
        return false;
    }

    *existed = false;
    *reminder = MemoReminder();
    if (recordId <= 0) {
        return true;
    }

    if (MemoRepository::getReminderByRecordId(recordId, reminder)) {
        *existed = true;
        return true;
    }

    return MemoRepository::lastError().isEmpty();
}

bool MemoTreeController::openRecordEditor(MemoRecord* record,
                                          MemoReminder* reminder,
                                          bool* shouldPersistReminder,
                                          bool reminderExisted,
                                          bool createMode) {
    if (record == nullptr || reminder == nullptr || shouldPersistReminder == nullptr) {
        return false;
    }

    EditTextDialog dialog(nullptr);
    if (createMode) {
        dialog.setupForCreate(*record);
    } else {
        dialog.setupForEdit(*record, reminderExisted ? reminder : nullptr);
    }

    if (dialog.exec() != QDialog::Accepted || !dialog.shouldPersistRecord()) {
        return false;
    }

    *record = dialog.resultRecord();
    *reminder = dialog.resultReminder();
    *shouldPersistReminder = dialog.shouldPersistReminder();
    return true;
}

bool MemoTreeController::persistReminderForRecord(const MemoReminder& reminder, bool reminderExisted) {
    if (!reminderExisted) {
        return !reminder.isEnabled || MemoRepository::addReminder(reminder, nullptr);
    }

    return MemoRepository::updateReminder(reminder);
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
    record.title = QStringLiteral("");
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
