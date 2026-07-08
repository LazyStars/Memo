#pragma once

#include "database/memogroup.h"
#include "database/memorecord.h"

#include <qalgorithms.h>
#include <qlist.h>

enum class MemoTreeNodeType {
    Root = 0,
    Group = 1,
    Record = 2
};

class MemoTreeNode
{
public:
    explicit MemoTreeNode(MemoTreeNodeType type, MemoTreeNode* parentNode = nullptr)
        : nodeType(type), parent(parentNode) {
    }

    ~MemoTreeNode() {
        clearChildren();
    }

    void appendChild(MemoTreeNode* childNode) {
        if (childNode == nullptr) {
            return;
        }

        childNode->parent = this;
        children.append(childNode);
    }

    void clearChildren() {
        qDeleteAll(children);
        children.clear();
    }

    MemoTreeNode* childAt(int row) const {
        if (row < 0 || row >= children.size()) {
            return nullptr;
        }

        return children.at(row);
    }

    int childCount() const {
        return children.size();
    }

    int row() const {
        if (parent == nullptr) {
            return 0;
        }

        return parent->children.indexOf(const_cast<MemoTreeNode*>(this));
    }

    MemoTreeNode* parentNode() const {
        return parent;
    }

    MemoTreeNodeType type() const {
        return nodeType;
    }

    void setGroup(const MemoGroup& value) {
        group = value;
    }

    const MemoGroup& groupData() const {
        return group;
    }

    void setRecord(const MemoRecord& value) {
        record = value;
    }

    const MemoRecord& recordData() const {
        return record;
    }

private:
    MemoTreeNodeType nodeType = MemoTreeNodeType::Root;
    MemoTreeNode* parent = nullptr;
    QList<MemoTreeNode*> children;
    MemoGroup group;
    MemoRecord record;
};
