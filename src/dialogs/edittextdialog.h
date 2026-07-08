#pragma once

#include "database/memorecord.h"
#include "database/memoreminder.h"
#include "ui_edittextdialog.h"

#include <qdialog.h>

class QCloseEvent;

class EditTextDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditTextDialog(QWidget* parent = nullptr);

    void setupForCreate(const MemoRecord& record);
    void setupForEdit(const MemoRecord& record, const MemoReminder* reminder = nullptr);

    bool shouldPersistRecord() const;
    MemoRecord resultRecord() const;
    bool shouldPersistReminder() const;
    MemoReminder resultReminder() const;

private slots:
    void on_btn_set_clicked();
    void on_btn_back_clicked();
    void on_btn_confirm_clicked();
    void on_btn_save_clicked();
    void on_btn_cancel_clicked();

private:
    struct DialogState {
        QString title;
        QString contentHtml;
        QString contentPlain;
        MemoStatus status = MemoStatus::NotStarted;
        bool reminderEnabled = false;
        int timingMinutes = 0;
        bool repeatEnabled = false;
        bool urgeEnabled = false;
        int urgeMinutes = 0;
    };

    void closeEvent(QCloseEvent* event) override;
    void reject() override;

    void connectSignals();
    void initializeDialog(const MemoRecord& record, const MemoReminder* reminder, bool createMode);
    void applyStateToUi(const DialogState& state);
    void captureSettingsPageSnapshot();
    void restoreSettingsPageSnapshot();
    void updateSavedState();
    void updateTitleCounter();
    void updateReminderWidgetsState();
    void updateDialogResult(bool persistRecord);
    void switchToContentPage();
    void switchToSettingsPage();
    bool trySaveAndClose();
    bool tryCloseByUser();
    bool validateBeforeLeavingSettingsPage();
    bool validateBeforePersist();
    bool buildCurrentOutput(MemoRecord* record, MemoReminder* reminder);
    DialogState collectStateFromUi() const;
    DialogState stateFromData(const MemoRecord& record, const MemoReminder* reminder) const;
    bool statesMatch(const DialogState& lhs, const DialogState& rhs) const;
    int comboIndexForStatus(MemoStatus status) const;
    MemoStatus statusForComboIndex(int index) const;
    qint64 minutesToSeconds(int minutes) const;

private:
    Ui::EditTextDialog ui;
    MemoRecord sourceRecord;
    MemoReminder sourceReminder;
    MemoRecord outputRecord;
    MemoReminder outputReminder;
    DialogState lastSavedState;
    DialogState settingsPageSnapshot;
    bool createMode = true;
    bool saved = true;
    bool persistRecord = false;
    bool persistReminder = false;
    bool reminderExisted = false;
    bool applyingUiState = false;
    bool allowClose = false;
};
