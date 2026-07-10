#include "edittextdialog.h"

#include <qevent.h>
#include <qdatetime.h>
#include <qmessagebox.h>
#include <qtimer.h>
#include <QWKWidgets/widgetwindowagent.h>

EditTextDialog::EditTextDialog(QWidget* parent) : QDialog(parent) {
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    ui.setupUi(this);

    auto agent = new QWK::WidgetWindowAgent(this);
    agent->setup(this);
    agent->setTitleBar(ui.widget_title);
    agent->setSystemButton(QWK::WindowAgentBase::WindowIcon, ui.label_title);
    agent->setSystemButton(QWK::WindowAgentBase::Close, ui.btn_back);
    agent->setHitTestVisible(ui.btn_set, true);

    ui.lineEdit_timing->setValidator(new QIntValidator(1, 525600, this));
    ui.lineEdit_urge->setValidator(new QIntValidator(1, 525600, this));
    ui.cb_popup->setVisible(false); // 暂未接入持久化能力，本次先隐藏
    ui.lineEdit_title->installEventFilter(this);
    ui.textEdit->installEventFilter(this);

    switchToContentPage();
    updateTitleCounter();
    updateReminderWidgetsState();
}

void EditTextDialog::setupForCreate(const MemoRecord& record) {
    initializeDialog(record, nullptr, true);
}

void EditTextDialog::setupForEdit(const MemoRecord& record, const MemoReminder* reminder) {
    initializeDialog(record, reminder, false);
}

bool EditTextDialog::shouldPersistRecord() const {
    return persistRecord;
}

MemoRecord EditTextDialog::resultRecord() const {
    return outputRecord;
}

bool EditTextDialog::shouldPersistReminder() const {
    return persistReminder;
}

MemoReminder EditTextDialog::resultReminder() const {
    return outputReminder;
}

void EditTextDialog::on_btn_set_clicked() {
    switchToSettingsPage();
}

void EditTextDialog::on_btn_back_clicked() {
    close();
}

void EditTextDialog::on_btn_return_clicked() {
    close();
}

void EditTextDialog::on_btn_confirm_clicked() {
    (void)trySaveAndClose();
}

void EditTextDialog::on_btn_save_clicked() {
    if (!validateBeforeLeavingSettingsPage()) {
        return;
    }

    switchToContentPage();
}

void EditTextDialog::on_btn_cancel_clicked() {
    restoreSettingsPageSnapshot();
    switchToContentPage();
}

void EditTextDialog::on_lineEdit_title_textChanged(const QString& text) {
    Q_UNUSED(text)
    updateTitleCounter();
    updateSavedState();
}

void EditTextDialog::on_textEdit_textChanged() {
    updateSavedState();
}

void EditTextDialog::on_comboBox_state_currentIndexChanged(int index) {
    Q_UNUSED(index)
    updateSavedState();
}

void EditTextDialog::on_cb_timing_toggled(bool checked) {
    Q_UNUSED(checked)
    updateReminderWidgetsState();
    updateSavedState();
}

void EditTextDialog::on_cb_repeat_toggled(bool checked) {
    Q_UNUSED(checked)
    updateReminderWidgetsState();
    updateSavedState();
}

void EditTextDialog::on_cb_urge_toggled(bool checked) {
    Q_UNUSED(checked)
    updateReminderWidgetsState();
    updateSavedState();
}

void EditTextDialog::on_lineEdit_timing_textChanged(const QString& text) {
    Q_UNUSED(text)
    updateSavedState();
}

void EditTextDialog::on_lineEdit_urge_textChanged(const QString& text) {
    Q_UNUSED(text)
    updateSavedState();
}

void EditTextDialog::closeEvent(QCloseEvent* event) {
    if (allowClose) {
        event->accept();
        return;
    }

    if (tryCloseByUser()) {
        event->accept();
        return;
    }

    event->ignore();
}

void EditTextDialog::reject() {
    close();
}

void EditTextDialog::initializeDialog(const MemoRecord& record, const MemoReminder* reminder, bool isCreateMode) {
    sourceRecord = record;
    sourceReminder = reminder == nullptr ? MemoReminder() : *reminder;
    outputRecord = record;
    outputReminder = sourceReminder;
    createMode = isCreateMode;
    reminderExisted = reminder != nullptr;
    persistRecord = false;
    persistReminder = false;
    allowClose = false;

    applyStateToUi(stateFromData(record, reminder));
    lastSavedState = collectStateFromUi();
    settingsPageSnapshot = lastSavedState;
    saved = true;
    switchToContentPage();
    ui.lineEdit_title->setFocus();
    ui.lineEdit_title->selectAll();
}

void EditTextDialog::applyStateToUi(const DialogState& state) {
    applyingUiState = true;

    const QSignalBlocker blockTitle(ui.lineEdit_title);
    const QSignalBlocker blockText(ui.textEdit);
    const QSignalBlocker blockStatus(ui.comboBox_state);
    const QSignalBlocker blockTiming(ui.cb_timing);
    const QSignalBlocker blockRepeat(ui.cb_repeat);
    const QSignalBlocker blockUrge(ui.cb_urge);
    const QSignalBlocker blockTimingInput(ui.lineEdit_timing);
    const QSignalBlocker blockUrgeInput(ui.lineEdit_urge);

    ui.lineEdit_title->setText(state.title);
    if (!state.contentHtml.isEmpty()) {
        ui.textEdit->setHtml(state.contentHtml);
    } else {
        ui.textEdit->setPlainText(state.contentPlain);
    }
    ui.comboBox_state->setCurrentIndex(comboIndexForStatus(state.status));
    ui.cb_timing->setChecked(state.reminderEnabled);
    ui.cb_repeat->setChecked(state.repeatEnabled);
    ui.cb_urge->setChecked(state.urgeEnabled);
    ui.lineEdit_timing->setText(state.timingMinutes > 0 ? QString::number(state.timingMinutes) : QString());
    ui.lineEdit_urge->setText(state.urgeMinutes > 0 ? QString::number(state.urgeMinutes) : QString());

    applyingUiState = false;

    updateTitleCounter();
    updateReminderWidgetsState();
    updateSavedState();
}

void EditTextDialog::captureSettingsPageSnapshot() {
    settingsPageSnapshot = collectStateFromUi();
}

void EditTextDialog::restoreSettingsPageSnapshot() {
    applyStateToUi(settingsPageSnapshot);
}

void EditTextDialog::updateSavedState() {
    if (applyingUiState) {
        return;
    }

    saved = statesMatch(collectStateFromUi(), lastSavedState);
}

void EditTextDialog::updateTitleCounter() {
    ui.label_limit->setText(QStringLiteral("%1/%2")
                                .arg(ui.lineEdit_title->text().size())
                                .arg(ui.lineEdit_title->maxLength()));
}

void EditTextDialog::updateReminderWidgetsState() {
    const bool timingEnabled = ui.cb_timing->isChecked();
    const bool urgeEnabled = timingEnabled && ui.cb_urge->isChecked();

    ui.widget->setEnabled(timingEnabled);
    ui.cb_repeat->setEnabled(timingEnabled);
    ui.cb_urge->setEnabled(timingEnabled);
    ui.widget_2->setEnabled(urgeEnabled);
}

void EditTextDialog::updateDialogResult(bool shouldPersist) {
    persistRecord = shouldPersist;
    if (!shouldPersist) {
        persistReminder = false;
    }
    allowClose = true;
    setResult(shouldPersist ? QDialog::Accepted : QDialog::Rejected);
}

void EditTextDialog::switchToContentPage() {
    if (ui.stackedWidget->currentWidget() != ui.page_context) {
        ui.stackedWidget->setCurrentWidget(ui.page_context);
    }
}

void EditTextDialog::switchToSettingsPage() {
    captureSettingsPageSnapshot();
    if (ui.stackedWidget->currentWidget() != ui.page_set) {
        ui.stackedWidget->setCurrentWidget(ui.page_set);
    }
}

bool EditTextDialog::trySaveAndClose() {
    MemoRecord record;
    MemoReminder reminder;
    if (!buildCurrentOutput(&record, &reminder)) {
        return false;
    }

    outputRecord = record;
    outputReminder = reminder;
    updateDialogResult(true);
    close();
    return true;
}

bool EditTextDialog::tryCloseByUser() {
    const DialogState currentState = collectStateFromUi();
    if (saved) {
        updateDialogResult(false);
        return true;
    }

    if (!createMode && currentState.title.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("标题不能为空。"));
        ui.lineEdit_title->setFocus();
        return false;
    }

    if (createMode && currentState.title.isEmpty()) {
        updateDialogResult(false);
        return true;
    }

    const QMessageBox::StandardButton button = QMessageBox::question(
        this,
        QStringLiteral("提示"),
        QStringLiteral("当前修改未保存，是否保存后关闭？"),
        QMessageBox::Save | QMessageBox::Close,
        QMessageBox::Save);
    if (button == QMessageBox::Save) {
        return trySaveAndClose();
    }

    updateDialogResult(false);
    return true;
}

bool EditTextDialog::validateBeforeLeavingSettingsPage() {
    if (!ui.cb_timing->isChecked()) {
        return true;
    }

    const int timingMinutes = ui.lineEdit_timing->text().toInt();
    if (timingMinutes <= 0) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请填写有效的提醒时间。"));
        ui.lineEdit_timing->setFocus();
        return false;
    }

    if (!ui.cb_urge->isChecked()) {
        return true;
    }

    const int urgeMinutes = ui.lineEdit_urge->text().toInt();
    if (urgeMinutes <= 0) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请填写有效的督促提醒时间。"));
        ui.lineEdit_urge->setFocus();
        return false;
    }

    if (urgeMinutes > timingMinutes) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("督促提醒时间不能大于提醒时间。"));
        ui.lineEdit_urge->setFocus();
        return false;
    }

    return true;
}

bool EditTextDialog::validateBeforePersist() {
    if (ui.lineEdit_title->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("标题不能为空。"));
        ui.lineEdit_title->setFocus();
        return false;
    }

    return validateBeforeLeavingSettingsPage();
}

bool EditTextDialog::buildCurrentOutput(MemoRecord* record, MemoReminder* reminder) {
    if (record == nullptr || reminder == nullptr) {
        return false;
    }

    if (!validateBeforePersist()) {
        return false;
    }

    const DialogState currentState = collectStateFromUi();
    const bool timingChanged = currentState.reminderEnabled != lastSavedState.reminderEnabled
                               || currentState.timingMinutes != lastSavedState.timingMinutes
                               || currentState.repeatEnabled != lastSavedState.repeatEnabled
                               || currentState.urgeEnabled != lastSavedState.urgeEnabled
                               || currentState.urgeMinutes != lastSavedState.urgeMinutes;
    const qint64 now = QDateTime::currentSecsSinceEpoch();

    MemoRecord nextRecord = sourceRecord;
    nextRecord.title = currentState.title;
    nextRecord.contentHtml = currentState.contentHtml;
    nextRecord.contentPlain = currentState.contentPlain;
    nextRecord.status = currentState.status;
    nextRecord.updatedAt = now;
    if (currentState.status == MemoStatus::Completed) {
        nextRecord.completedAt = sourceRecord.completedAt > 0 ? sourceRecord.completedAt : now;
    } else {
        nextRecord.completedAt = 0;
    }

    MemoReminder nextReminder = sourceReminder;
    const qint64 finishWithinSeconds = minutesToSeconds(currentState.timingMinutes);
    if (currentState.reminderEnabled) {
        nextRecord.planDelaySeconds = finishWithinSeconds;
        nextRecord.planDueAt = reminderExisted && !timingChanged && sourceRecord.planDueAt > 0
                               ? sourceRecord.planDueAt
                               : now + finishWithinSeconds;

        nextReminder.recordId = sourceRecord.id;
        nextReminder.finishWithinSeconds = finishWithinSeconds;
        nextReminder.repeatMode = currentState.repeatEnabled
                                      ? MemoReminderRepeatMode::Repeat
                                      : MemoReminderRepeatMode::Once;
        nextReminder.remindIntervalSeconds = currentState.repeatEnabled
                                                 ? qMax<qint64>(
                                                       60,
                                                       currentState.urgeEnabled && currentState.urgeMinutes > 0
                                                           ? minutesToSeconds(currentState.urgeMinutes)
                                                           : finishWithinSeconds)
                                                 : 0;
        nextReminder.isEnabled = true;
        nextReminder.updatedAt = now;
        if (!reminderExisted) {
            nextReminder.createdAt = now;
        }

        if (reminderExisted && !timingChanged && sourceReminder.dueAt > 0) {
            nextReminder.startRemindAt = sourceReminder.startRemindAt;
            nextReminder.dueAt = sourceReminder.dueAt;
            nextReminder.nextRemindAt = sourceReminder.nextRemindAt;
            nextReminder.lastRemindedAt = sourceReminder.lastRemindedAt;
        } else {
            const qint64 urgeLeadSeconds = currentState.urgeEnabled ? minutesToSeconds(currentState.urgeMinutes) : 0;
            nextReminder.dueAt = nextRecord.planDueAt;
            nextReminder.startRemindAt = urgeLeadSeconds > 0
                                             ? qMax(now, nextRecord.planDueAt - urgeLeadSeconds)
                                             : nextRecord.planDueAt;
            nextReminder.nextRemindAt = nextReminder.startRemindAt;
            nextReminder.lastRemindedAt = 0;
        }
    } else {
        nextRecord.planDelaySeconds = 0;
        nextRecord.planDueAt = 0;

        nextReminder.recordId = sourceRecord.id;
        nextReminder.startRemindAt = 0;
        nextReminder.dueAt = 0;
        nextReminder.finishWithinSeconds = 0;
        nextReminder.remindIntervalSeconds = 0;
        nextReminder.repeatMode = MemoReminderRepeatMode::Once;
        nextReminder.nextRemindAt = 0;
        nextReminder.lastRemindedAt = 0;
        nextReminder.isEnabled = false;
        nextReminder.updatedAt = now;
        if (reminderExisted) {
            nextReminder.createdAt = sourceReminder.createdAt;
        }
    }

    *record = nextRecord;
    *reminder = nextReminder;
    persistReminder = currentState.reminderEnabled || reminderExisted;
    return true;
}

EditTextDialog::DialogState EditTextDialog::collectStateFromUi() const {
    DialogState state;
    state.title = ui.lineEdit_title->text().trimmed();
    state.contentHtml = ui.textEdit->toHtml();
    state.contentPlain = ui.textEdit->toPlainText();
    state.status = statusForComboIndex(ui.comboBox_state->currentIndex());
    state.reminderEnabled = ui.cb_timing->isChecked();
    state.timingMinutes = state.reminderEnabled ? ui.lineEdit_timing->text().toInt() : 0;
    state.repeatEnabled = state.reminderEnabled && ui.cb_repeat->isChecked();
    state.urgeEnabled = state.reminderEnabled && ui.cb_urge->isChecked();
    state.urgeMinutes = state.urgeEnabled ? ui.lineEdit_urge->text().toInt() : 0;
    return state;
}

EditTextDialog::DialogState EditTextDialog::stateFromData(const MemoRecord& record, const MemoReminder* reminder) const {
    DialogState state;
    state.title = record.title.trimmed();
    state.contentHtml = record.contentHtml;
    state.contentPlain = record.contentPlain;
    state.status = record.status;

    if (reminder != nullptr && reminder->isEnabled) {
        state.reminderEnabled = true;
        const qint64 finishWithinSeconds = record.planDelaySeconds > 0
                                               ? record.planDelaySeconds
                                               : reminder->finishWithinSeconds;
        state.timingMinutes = finishWithinSeconds > 0
                                  ? static_cast<int>(qMax<qint64>(1, (finishWithinSeconds + 59) / 60))
                                  : 0;
        state.repeatEnabled = reminder->repeatMode == MemoReminderRepeatMode::Repeat
                              && reminder->remindIntervalSeconds > 0;
        const qint64 urgeLeadSeconds = reminder->dueAt > reminder->startRemindAt
                                           ? reminder->dueAt - reminder->startRemindAt
                                           : 0;
        state.urgeEnabled = urgeLeadSeconds > 0;
        state.urgeMinutes = urgeLeadSeconds > 0
                                ? static_cast<int>(qMax<qint64>(1, (urgeLeadSeconds + 59) / 60))
                                : 0;
    }

    return state;
}

bool EditTextDialog::statesMatch(const DialogState& lhs, const DialogState& rhs) const {
    return lhs.title == rhs.title
           && lhs.contentHtml == rhs.contentHtml
           && lhs.contentPlain == rhs.contentPlain
           && lhs.status == rhs.status
           && lhs.reminderEnabled == rhs.reminderEnabled
           && lhs.timingMinutes == rhs.timingMinutes
           && lhs.repeatEnabled == rhs.repeatEnabled
           && lhs.urgeEnabled == rhs.urgeEnabled
           && lhs.urgeMinutes == rhs.urgeMinutes;
}

int EditTextDialog::comboIndexForStatus(MemoStatus status) const {
    switch (status) {
    case MemoStatus::InProgress:
        return 0;
    case MemoStatus::Completed:
        return 2;
    case MemoStatus::Planned:
        return 3;
    case MemoStatus::TimedOut:
        return 4;
    case MemoStatus::NotStarted:
    default:
        return 1;
    }
}

MemoStatus EditTextDialog::statusForComboIndex(int index) const {
    switch (index) {
    case 0:
        return MemoStatus::InProgress;
    case 2:
        return MemoStatus::Completed;
    case 3:
        return MemoStatus::Planned;
    case 4:
        return MemoStatus::TimedOut;
    case 1:
    default:
        return MemoStatus::NotStarted;
    }
}

qint64 EditTextDialog::minutesToSeconds(int minutes) const {
    return static_cast<qint64>(qMax(0, minutes)) * 60;
}

bool EditTextDialog::eventFilter(QObject* watched, QEvent* event) {
    const bool watchesEditableContent = watched == ui.lineEdit_title || watched == ui.textEdit;
    if (!watchesEditableContent) {
        return QDialog::eventFilter(watched, event);
    }

    switch (event->type()) {
    case QEvent::KeyRelease:
    case QEvent::InputMethod:
    case QEvent::MouseButtonRelease:
        QTimer::singleShot(0, this, [this] {
            updateTitleCounter();
            updateSavedState();
        });
        break;
    default:
        break;
    }

    return QDialog::eventFilter(watched, event);
}
