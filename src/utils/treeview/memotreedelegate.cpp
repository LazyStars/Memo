#include "memotreedelegate.h"

#include "database/memorecord.h"
#include "memotreemodel.h"

#include <qabstractitemmodel.h>
#include <qevent.h>
#include <qfont.h>
#include <qfontmetrics.h>
#include <qlineedit.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qstyle.h>
#include <qtreeview.h>

MemoTreeDelegate::MemoTreeDelegate(QObject* parent) : QStyledItemDelegate(parent) {
}

void MemoTreeDelegate::setTreeView(QTreeView* view) {
    if (treeView != nullptr && treeView->viewport() != nullptr) {
        treeView->viewport()->removeEventFilter(this);
    }

    treeView = view;
    clearInteractionState();

    if (treeView != nullptr && treeView->viewport() != nullptr) {
        treeView->viewport()->installEventFilter(this);
    }
}

void MemoTreeDelegate::paint(QPainter* painter,
                             const QStyleOptionViewItem& option,
                             const QModelIndex& index) const {
    const auto* treeModel = qobject_cast<const MemoTreeModel*>(index.model());
    if (treeModel == nullptr) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    const MemoTreeNode* node = treeModel->nodeFromIndex(index);
    if (node == nullptr) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    switch (node->type()) {
    case MemoTreeNodeType::Group:
        paintGroupNode(painter, option, index, node);
        break;
    case MemoTreeNodeType::Record:
        paintRecordNode(painter, option, node);
        break;
    case MemoTreeNodeType::Root:
    default:
        QStyledItemDelegate::paint(painter, option, index);
        break;
    }

    painter->restore();
}

QSize MemoTreeDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    Q_UNUSED(option);

    const auto* treeModel = qobject_cast<const MemoTreeModel*>(index.model());
    if (treeModel == nullptr) {
        return {0, 0};
    }

    const MemoTreeNode* node = treeModel->nodeFromIndex(index);
    if (node == nullptr) {
        return {0, 0};
    }

    if (node->type() == MemoTreeNodeType::Group) {
        return {0, 40};
    }

    if (node->type() == MemoTreeNodeType::Record) {
        return {0, 84};
    }

    return {0, 0};
}

QWidget* MemoTreeDelegate::createEditor(QWidget* parent,
                                        const QStyleOptionViewItem& option,
                                        const QModelIndex& index) const {
    Q_UNUSED(option);

    const auto* treeModel = qobject_cast<const MemoTreeModel*>(index.model());
    if (treeModel == nullptr) {
        return nullptr;
    }

    const MemoTreeNode* node = treeModel->nodeFromIndex(index);
    if (node == nullptr || node->type() != MemoTreeNodeType::Group) {
        return nullptr;
    }

    auto* editor = new QLineEdit(parent);
    editor->setFrame(false);
    return editor;
}

void MemoTreeDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const {
    auto* lineEdit = qobject_cast<QLineEdit*>(editor);
    if (lineEdit == nullptr) {
        return;
    }

    lineEdit->setText(index.data(Qt::EditRole).toString());
    lineEdit->selectAll();
}

void MemoTreeDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const {
    auto* lineEdit = qobject_cast<QLineEdit*>(editor);
    if (lineEdit == nullptr || model == nullptr) {
        return;
    }

    const auto* treeModel = qobject_cast<const MemoTreeModel*>(index.model());
    if (treeModel == nullptr) {
        return;
    }

    const MemoTreeNode* node = treeModel->nodeFromIndex(index);
    if (node == nullptr || node->type() != MemoTreeNodeType::Group) {
        return;
    }

    QString groupName = lineEdit->text().trimmed();
    if (groupName.isEmpty()) {
        groupName = node->groupData().name;
    }

    model->setData(index, groupName, Qt::EditRole);
    Q_EMIT const_cast<MemoTreeDelegate*>(this)->requestRenameGroup(node->groupData().id, groupName);
}

void MemoTreeDelegate::updateEditorGeometry(QWidget* editor,
                                            const QStyleOptionViewItem& option,
                                            const QModelIndex& index) const {
    Q_UNUSED(index);

    if (editor == nullptr) {
        return;
    }

    editor->setGeometry(groupTextRect(option.rect).adjusted(0, 4, 0, -4));
}

bool MemoTreeDelegate::editorEvent(QEvent* event,
                                   QAbstractItemModel* model,
                                   const QStyleOptionViewItem& option,
                                   const QModelIndex& index) {
    Q_UNUSED(model);

    const auto* treeModel = qobject_cast<const MemoTreeModel*>(index.model());
    if (treeModel == nullptr) {
        return QStyledItemDelegate::editorEvent(event, model, option, index);
    }

    const MemoTreeNode* node = treeModel->nodeFromIndex(index);
    if (node == nullptr || node->type() != MemoTreeNodeType::Group) {
        return QStyledItemDelegate::editorEvent(event, model, option, index);
    }

    const GroupControl control = controlAt(static_cast<QMouseEvent*>(event)->pos(), option.rect);

    if (event->type() == QEvent::MouseMove) {
        updateHoveredControl(index, control);
        return control != GroupControl::None;
    }

    if (event->type() == QEvent::MouseButtonPress) {
        auto* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton && control != GroupControl::None) {
            updateHoveredControl(index, control);
            updatePressedControl(index, control);
            return true;
        }

        updatePressedControl({}, GroupControl::None);
        return QStyledItemDelegate::editorEvent(event, model, option, index);
    }

    if (event->type() == QEvent::MouseButtonRelease) {
        auto* mouseEvent = static_cast<QMouseEvent*>(event);
        const bool isTriggered = mouseEvent->button() == Qt::LeftButton
                                 && pressedIndex == index
                                 && pressedControl == control
                                 && control != GroupControl::None;

        updatePressedControl({}, GroupControl::None);
        updateHoveredControl(index, control);

        if (!isTriggered) {
            return control != GroupControl::None;
        }

        if (control == GroupControl::AddButton) {
            Q_EMIT requestAddRecord(node->groupData().id);
            return true;
        }

        if (control == GroupControl::ToggleArrow && treeView != nullptr) {
            if (treeView->isExpanded(index)) {
                treeView->collapse(index);
            } else {
                treeView->expand(index);
            }
            return true;
        }
    }

    return QStyledItemDelegate::editorEvent(event, model, option, index);
}

void MemoTreeDelegate::paintGroupNode(QPainter* painter,
                                      const QStyleOptionViewItem& option,
                                      const QModelIndex& index,
                                      const MemoTreeNode* node) const {
    const QRect contentRect = groupContentRect(option.rect);
    const bool isSelected = (option.state & QStyle::State_Selected) != 0;

    QColor backgroundColor = QColor(78, 88, 104, 70);
    QColor borderColor = QColor(170, 180, 200, 90);
    QColor textColor = QColor(236, 239, 244);
    if (isSelected) {
        backgroundColor = QColor(92, 122, 170, 130);
        borderColor = QColor(180, 205, 255, 150);
    }

    painter->setPen(QPen(borderColor, 1));
    painter->setBrush(backgroundColor);
    painter->drawRoundedRect(contentRect, 8, 8);

    QFont titleFont = option.font;
    titleFont.setBold(true);
    painter->setFont(titleFont);
    painter->setPen(textColor);

    const QString title = node->groupData().name;
    const QRect textRect = groupTextRect(contentRect);
    const QString displayText = QFontMetrics(titleFont).elidedText(title, Qt::ElideRight, textRect.width());
    painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, displayText);

    const QRect arrowRect = groupArrowRect(contentRect);
    const bool isExpanded = treeView != nullptr && treeView->isExpanded(index);
    painter->drawPixmap(arrowRect, QPixmap(groupArrowIconPath(isExpanded, GroupControl::ToggleArrow, index)));

    const QRect addRect = groupAddButtonRect(contentRect);
    painter->drawPixmap(addRect, QPixmap(groupAddIconPath(GroupControl::AddButton, index)));
}

void MemoTreeDelegate::paintRecordNode(QPainter* painter,
                                       const QStyleOptionViewItem& option,
                                       const MemoTreeNode* node) const {
    const MemoRecord& record = node->recordData();
    const QRect contentRect = option.rect.adjusted(8, 4, -8, -4);
    const bool isSelected = (option.state & QStyle::State_Selected) != 0;

    QColor backgroundColor = QColor(48, 54, 66, 82);
    QColor borderColor = QColor(255, 255, 255, 32);
    QColor titleColor = QColor(240, 243, 248);
    QColor bodyColor = QColor(196, 202, 212);
    if (isSelected) {
        backgroundColor = QColor(70, 96, 138, 125);
        borderColor = QColor(180, 205, 255, 120);
    }

    painter->setPen(QPen(borderColor, 1));
    painter->setBrush(backgroundColor);
    painter->drawRoundedRect(contentRect, 10, 10);

    QFont titleFont = option.font;
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 1);
    painter->setFont(titleFont);
    painter->setPen(titleColor);

    const QRect titleRect = contentRect.adjusted(12, 8, -84, -36);
    const QString title = record.title.isEmpty() ? QStringLiteral("未命名标题") : record.title;
    painter->drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter,
                      QFontMetrics(titleFont).elidedText(title, Qt::ElideRight, titleRect.width()));

    const QRect statusRect(contentRect.right() - 72, contentRect.top() + 8, 60, 20);
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(255, 255, 255, isSelected ? 52 : 36));
    painter->drawRoundedRect(statusRect, 10, 10);

    QFont statusFont = option.font;
    statusFont.setPointSize(qMax(8, statusFont.pointSize() - 1));
    painter->setFont(statusFont);
    painter->setPen(QColor(236, 239, 244));
    painter->drawText(statusRect, Qt::AlignCenter, statusText(static_cast<int>(record.status)));

    QFont bodyFont = option.font;
    painter->setFont(bodyFont);
    painter->setPen(bodyColor);

    const QRect bodyRect = contentRect.adjusted(12, 34, -12, -10);
    const QString body = record.contentPlain.simplified();
    const QString summary = body.isEmpty() ? QStringLiteral("暂无内容") : body;
    painter->drawText(bodyRect, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                      QFontMetrics(bodyFont).elidedText(summary, Qt::ElideRight, bodyRect.width() * 2));
}

QString MemoTreeDelegate::statusText(int status) const {
    switch (static_cast<MemoStatus>(status)) {
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

QString MemoTreeDelegate::groupArrowIconPath(bool expanded,
                                             GroupControl control,
                                             const QModelIndex& index) const {
    QString iconPath = expanded
        ? QStringLiteral(":/btnicon/res/btnicon/arrow_down.png")
        : QStringLiteral(":/btnicon/res/btnicon/arrow_up.png");

    if (pressedIndex == index && pressedControl == control) {
        return iconPath.replace(QStringLiteral(".png"), QStringLiteral("-pressed.png"));
    }

    if (hoveredIndex == index && hoveredControl == control) {
        return iconPath.replace(QStringLiteral(".png"), QStringLiteral("-hover.png"));
    }

    return iconPath;
}

QString MemoTreeDelegate::groupAddIconPath(GroupControl control, const QModelIndex& index) const {
    QString iconPath = QStringLiteral(":/btnicon/res/btnicon/add.png");
    if (pressedIndex == index && pressedControl == control) {
        return iconPath.replace(QStringLiteral(".png"), QStringLiteral("-pressed.png"));
    }

    if (hoveredIndex == index && hoveredControl == control) {
        return iconPath.replace(QStringLiteral(".png"), QStringLiteral("-hover.png"));
    }

    return iconPath;
}

MemoTreeDelegate::GroupControl MemoTreeDelegate::controlAt(const QPoint& position, const QRect& itemRect) const {
    const QRect contentRect = groupContentRect(itemRect);
    if (groupArrowRect(contentRect).contains(position)) {
        return GroupControl::ToggleArrow;
    }

    if (groupAddButtonRect(contentRect).contains(position)) {
        return GroupControl::AddButton;
    }

    return GroupControl::None;
}

QRect MemoTreeDelegate::groupContentRect(const QRect& rect) const {
    return rect.adjusted(4, 4, -4, -4);
}

QRect MemoTreeDelegate::groupArrowRect(const QRect& rect) const {
    return QRect(rect.left() + 10, rect.center().y() - 8, 16, 16);
}

QRect MemoTreeDelegate::groupTextRect(const QRect& rect) const {
    return rect.adjusted(34, 0, -44, 0);
}

QRect MemoTreeDelegate::groupAddButtonRect(const QRect& rect) const {
    return QRect(rect.right() - 28, rect.center().y() - 8, 16, 16);
}

void MemoTreeDelegate::updateHoveredControl(const QModelIndex& index, GroupControl control) {
    const QPersistentModelIndex previousIndex = hoveredIndex;
    const GroupControl previousControl = hoveredControl;

    hoveredIndex = control == GroupControl::None ? QPersistentModelIndex() : QPersistentModelIndex(index);
    hoveredControl = control;

    if (previousIndex != hoveredIndex || previousControl != hoveredControl) {
        updateIndexVisual(previousIndex);
        updateIndexVisual(hoveredIndex);
    }
}

void MemoTreeDelegate::updatePressedControl(const QModelIndex& index, GroupControl control) {
    const QPersistentModelIndex previousIndex = pressedIndex;
    const GroupControl previousControl = pressedControl;

    pressedIndex = control == GroupControl::None ? QPersistentModelIndex() : QPersistentModelIndex(index);
    pressedControl = control;

    if (previousIndex != pressedIndex || previousControl != pressedControl) {
        updateIndexVisual(previousIndex);
        updateIndexVisual(pressedIndex);
    }
}

void MemoTreeDelegate::clearInteractionState() {
    updateHoveredControl({}, GroupControl::None);
    updatePressedControl({}, GroupControl::None);
}

void MemoTreeDelegate::updateIndexVisual(const QModelIndex& index) {
    if (treeView == nullptr || !index.isValid() || treeView->viewport() == nullptr) {
        return;
    }

    treeView->viewport()->update(treeView->visualRect(index));
}

bool MemoTreeDelegate::eventFilter(QObject* watched, QEvent* event) {
    if (treeView == nullptr || watched != treeView->viewport()) {
        return QStyledItemDelegate::eventFilter(watched, event);
    }

    if (event->type() == QEvent::Leave) {
        clearInteractionState();
    }

    if (event->type() == QEvent::MouseMove
        || event->type() == QEvent::MouseButtonPress
        || event->type() == QEvent::MouseButtonRelease) {
        auto* mouseEvent = static_cast<QMouseEvent*>(event);
        if (!treeView->indexAt(mouseEvent->pos()).isValid()) {
            clearInteractionState();
        }
    }

    return QStyledItemDelegate::eventFilter(watched, event);
}
