#include "groupitem.h"

#include <qevent.h>
#include <qlineedit.h>

GroupItem::GroupItem(QWidget* parent) : QWidget(parent) {
    ui.setupUi(this);
    ui.lineEdit_group_name->setReadOnly(true);
    ui.lineEdit_group_name->installEventFilter(this);
    connect(ui.lineEdit_group_name, &QLineEdit::editingFinished, this, &GroupItem::onGroupNameEditingFinished);
}

void GroupItem::setGroup(const MemoGroup& group) {
    groupData = group;
    ui.lineEdit_group_name->setText(group.name);
}

void GroupItem::enterEditMode() {
    ui.lineEdit_group_name->setReadOnly(false);
    ui.lineEdit_group_name->setFocus();
    ui.lineEdit_group_name->selectAll();
}

bool GroupItem::eventFilter(QObject* watched, QEvent* event) {
    if (watched == ui.lineEdit_group_name
        && ui.lineEdit_group_name->isReadOnly()
        && event->type() == QEvent::MouseButtonDblClick) {
        enterEditMode();
        return true;
    }

    return QWidget::eventFilter(watched, event);
}

void GroupItem::on_btn_add_record_clicked() {
    emit requestAddRecord(groupData.id);
}

void GroupItem::onGroupNameEditingFinished() {
    if (ui.lineEdit_group_name->isReadOnly()) {
        return;
    }

    QString groupName = ui.lineEdit_group_name->text().trimmed();
    if (groupName.isEmpty()) {
        groupName = groupData.name;
    }

    ui.lineEdit_group_name->setText(groupName);
    finishEditing();

    if (groupName != groupData.name) {
        groupData.name = groupName;
        emit requestRenameGroup(groupData.id, groupName);
    }
}

void GroupItem::finishEditing() {
    ui.lineEdit_group_name->setReadOnly(true);
    ui.lineEdit_group_name->clearFocus();
}
