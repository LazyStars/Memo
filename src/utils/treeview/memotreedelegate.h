#pragma once

#include <qabstractitemmodel.h>
#include <qstyleditemdelegate.h>

class MemoTreeNode;
class QTreeView;

class MemoTreeDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit MemoTreeDelegate(QObject* parent = nullptr);
    void setTreeView(QTreeView* view);

    void paint(QPainter* painter,
               const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QWidget* createEditor(QWidget* parent,
                          const QStyleOptionViewItem& option,
                          const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
    void updateEditorGeometry(QWidget* editor,
                              const QStyleOptionViewItem& option,
                              const QModelIndex& index) const override;
    bool editorEvent(QEvent* event,
                     QAbstractItemModel* model,
                     const QStyleOptionViewItem& option,
                     const QModelIndex& index) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

signals:
    void requestAddRecord(qint64 groupId);
    void requestRenameGroup(qint64 groupId, const QString& groupName);

private:
    enum class GroupControl {
        None = 0,
        ToggleArrow,
        AddButton
    };

    void paintGroupNode(QPainter* painter,
                        const QStyleOptionViewItem& option,
                        const QModelIndex& index,
                        const MemoTreeNode* node) const;
    void paintRecordNode(QPainter* painter,
                         const QStyleOptionViewItem& option,
                         const MemoTreeNode* node) const;
    QString statusText(int status) const;
    QString groupArrowIconPath(bool expanded, GroupControl control, const QModelIndex& index) const;
    QString groupAddIconPath(GroupControl control, const QModelIndex& index) const;
    GroupControl controlAt(const QPoint& position, const QRect& itemRect) const;
    QRect groupContentRect(const QRect& rect) const;
    QRect groupArrowRect(const QRect& rect) const;
    QRect groupTextRect(const QRect& rect) const;
    QRect groupAddButtonRect(const QRect& rect) const;
    void updateHoveredControl(const QModelIndex& index, GroupControl control);
    void updatePressedControl(const QModelIndex& index, GroupControl control);
    void clearInteractionState();
    void updateIndexVisual(const QModelIndex& index);

private:
    QTreeView* treeView = nullptr;
    QPersistentModelIndex hoveredIndex;
    QPersistentModelIndex pressedIndex;
    GroupControl hoveredControl = GroupControl::None;
    GroupControl pressedControl = GroupControl::None;
};
