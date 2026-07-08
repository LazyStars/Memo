#include "memotreemodel.h"

#include <qhash.h>

MemoTreeModel::MemoTreeModel(QObject* parent) : QAbstractItemModel(parent) {
    rootNode = new MemoTreeNode(MemoTreeNodeType::Root);
}

MemoTreeModel::~MemoTreeModel() {
    delete rootNode;
    rootNode = nullptr;
}

QModelIndex MemoTreeModel::index(int row, int column, const QModelIndex& parentIndex) const {
    if (!hasIndex(row, column, parentIndex)) {
        return {};
    }

    MemoTreeNode* parentNode = nodeFromIndex(parentIndex);
    MemoTreeNode* childNode = parentNode->childAt(row);
    if (childNode == nullptr) {
        return {};
    }

    return createIndex(row, column, childNode);
}

QModelIndex MemoTreeModel::parent(const QModelIndex& child) const {
    if (!child.isValid()) {
        return {};
    }

    MemoTreeNode* childNode = nodeFromIndex(child);
    MemoTreeNode* parentNode = childNode == nullptr ? nullptr : childNode->parentNode();
    if (parentNode == nullptr || parentNode == rootNode) {
        return {};
    }

    return createIndex(parentNode->row(), 0, parentNode);
}

int MemoTreeModel::rowCount(const QModelIndex& parentIndex) const {
    if (parentIndex.column() > 0) {
        return 0;
    }

    MemoTreeNode* parentNode = nodeFromIndex(parentIndex);
    return parentNode->childCount();
}

int MemoTreeModel::columnCount(const QModelIndex& parent) const {
    Q_UNUSED(parent);
    return 1;
}

QVariant MemoTreeModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid()) {
        return {};
    }

    MemoTreeNode* node = nodeFromIndex(index);
    if (node == nullptr) {
        return {};
    }

    if (node->type() == MemoTreeNodeType::Group
        && (role == Qt::DisplayRole || role == Qt::EditRole)) {
        return node->groupData().name;
    }

    if (node->type() == MemoTreeNodeType::Record
        && (role == Qt::DisplayRole || role == Qt::EditRole)) {
        return node->recordData().title;
    }

    return {};
}

bool MemoTreeModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if (!index.isValid() || role != Qt::EditRole) {
        return false;
    }

    MemoTreeNode* node = nodeFromIndex(index);
    if (node == nullptr || node->type() != MemoTreeNodeType::Group) {
        return false;
    }

    MemoGroup group = node->groupData();
    group.name = value.toString();
    node->setGroup(group);
    emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
    return true;
}

Qt::ItemFlags MemoTreeModel::flags(const QModelIndex& index) const {
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }

    MemoTreeNode* node = nodeFromIndex(index);
    Qt::ItemFlags itemFlags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if (node != nullptr && node->type() == MemoTreeNodeType::Group) {
        itemFlags |= Qt::ItemIsEditable;
    }

    return itemFlags;
}

void MemoTreeModel::rebuild(const QList<MemoGroup>& groups, const QList<MemoRecord>& records) {
    beginResetModel();
    rootNode->clearChildren();

    QHash<qint64, MemoTreeNode*> groupNodeMap;
    for (const MemoGroup& group : groups) {
        auto groupNode = new MemoTreeNode(MemoTreeNodeType::Group, rootNode);
        groupNode->setGroup(group);
        rootNode->appendChild(groupNode);
        groupNodeMap.insert(group.id, groupNode);
    }

    MemoTreeNode* fallbackGroupNode = rootNode->childCount() > 0 ? rootNode->childAt(0) : nullptr;
    for (const MemoRecord& record : records) {
        MemoTreeNode* groupNode = groupNodeMap.value(record.groupId, fallbackGroupNode);
        if (groupNode == nullptr) {
            continue;
        }

        auto recordNode = new MemoTreeNode(MemoTreeNodeType::Record, groupNode);
        recordNode->setRecord(record);
        groupNode->appendChild(recordNode);
    }

    endResetModel();
}

MemoTreeNode* MemoTreeModel::nodeFromIndex(const QModelIndex& index) const {
    if (!index.isValid()) {
        return rootNode;
    }

    auto* node = static_cast<MemoTreeNode*>(index.internalPointer());
    return node == nullptr ? rootNode : node;
}

QModelIndex MemoTreeModel::groupIndex(qint64 groupId) const {
    for (int row = 0; row < rootNode->childCount(); ++row) {
        MemoTreeNode* groupNode = rootNode->childAt(row);
        if (groupNode != nullptr && groupNode->groupData().id == groupId) {
            return createIndex(row, 0, groupNode);
        }
    }

    return {};
}

QModelIndex MemoTreeModel::recordIndex(qint64 recordId) const {
    for (int groupRow = 0; groupRow < rootNode->childCount(); ++groupRow) {
        MemoTreeNode* groupNode = rootNode->childAt(groupRow);
        if (groupNode == nullptr) {
            continue;
        }

        for (int recordRow = 0; recordRow < groupNode->childCount(); ++recordRow) {
            MemoTreeNode* recordNode = groupNode->childAt(recordRow);
            if (recordNode != nullptr && recordNode->recordData().id == recordId) {
                const QModelIndex groupIndex = createIndex(groupRow, 0, groupNode);
                return index(recordRow, 0, groupIndex);
            }
        }
    }

    return {};
}
