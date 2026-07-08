#pragma once

#include "database/memorecord.h"
#include "ui_recorditem.h"

class RecordItem : public QWidget {
    Q_OBJECT

public:
    explicit RecordItem(QWidget* parent = nullptr);
    void setRecord(const MemoRecord& record);

private slots:
    void on_btn_edit_clicked();

private:
    Ui::RecordItem ui;
    MemoRecord recordData;
};
