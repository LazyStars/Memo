#pragma once

#include "ui_app.h"

#include <qtimer.h>

#include "tools/tray/mysystemtrayicon.h"

class MemoTreeController;

class App : public QWidget
{
    Q_OBJECT

public:
    explicit App(QWidget* parent = nullptr);

private:
    void initializeControl();
    void initializeTreeView();
    void startRealTimeTimer();

private slots:
    void on_btn_add_group_clicked();
    void on_btn_add_record_clicked();
    void on_btn_pin_top_clicked();
    void on_btn_hide_clicked();
    void on_btn_close_clicked();

private:
    Ui::App ui{};
    MySystemTrayIcon *trayIcon = nullptr;
    QTimer *updateTimer = nullptr;
    MemoTreeController* memoTreeController = nullptr;
};
