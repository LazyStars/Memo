#pragma once

#include "database/memogroup.h"
#include "ui_groupitem.h"

class GroupItem : public QWidget {
    Q_OBJECT

public:
    explicit GroupItem(QWidget* parent = nullptr);
    void setGroup(const MemoGroup& group);
    void enterEditMode();

signals:
    void requestAddRecord(qint64 groupId);
    void requestRenameGroup(qint64 groupId, const QString& groupName);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void on_btn_add_record_clicked();
    void onGroupNameEditingFinished();

private:
    void finishEditing();

    Ui::GroupItem ui;
    MemoGroup groupData;
};
