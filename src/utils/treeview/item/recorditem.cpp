#include "recorditem.h"

#include <qstyle.h>

RecordItem::RecordItem(QWidget* parent) : QWidget(parent) {
    ui.setupUi(this);
    ui.lineEdit_title->setReadOnly(true);
    ui.textEdit_context->setReadOnly(true);
}

void RecordItem::setRecord(const MemoRecord& record) {
    recordData = record;

    ui.lineEdit_title->setText(record.title.isEmpty() ? QStringLiteral("未命名标题") : record.title);
    ui.textEdit_context->setPlainText(record.contentPlain);

    QString stateText;
    QString stateSkin;
    switch (record.status) {
    case MemoStatus::InProgress:
        stateText = QStringLiteral("进行中");
        stateSkin = QStringLiteral("in_progress");
        break;
    case MemoStatus::Completed:
        stateText = QStringLiteral("已完成");
        stateSkin = QStringLiteral("completed");
        break;
    case MemoStatus::Planned:
        stateText = QStringLiteral("计划中");
        stateSkin = QStringLiteral("planned");
        break;
    case MemoStatus::TimedOut:
        stateText = QStringLiteral("已超时");
        stateSkin = QStringLiteral("delayed");
        break;
    case MemoStatus::NotStarted:
    default:
        stateText = QStringLiteral("未开始");
        stateSkin = QStringLiteral("not_started");
        break;
    }

    ui.label_state->setText(stateText);
    ui.label_state->setProperty("skin", stateSkin);
    ui.label_state->style()->unpolish(ui.label_state);
    ui.label_state->style()->polish(ui.label_state);
}

void RecordItem::on_btn_edit_clicked() {
    // 预留给后续更复杂的记录编辑能力
}
