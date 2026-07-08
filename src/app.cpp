#include "app.h"
#include "utils/treeview/memotreecontroller.h"

#include <qdatetime.h>
#include <QWKWidgets/widgetwindowagent.h>

App::App(QWidget* parent) : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    ui.setupUi(this);
    trayIcon = new MySystemTrayIcon(this);
    initializeControl();
}

void App::initializeControl() {
    auto agent = new QWK::WidgetWindowAgent(this);
    agent->setup(this);
    agent->setTitleBar(ui.widget_title);
    agent->setSystemButton(QWK::WindowAgentBase::WindowIcon, ui.label_title);
    agent->setSystemButton(QWK::WindowAgentBase::Minimize, ui.btn_hide);
    agent->setSystemButton(QWK::WindowAgentBase::Close, ui.btn_close);
    agent->setHitTestVisible(ui.btn_pin_top, true);

    initializeTreeView();
    startRealTimeTimer();
}

void App::initializeTreeView() {
    memoTreeController = new MemoTreeController(ui.treeView, this);
    memoTreeController->initialize();
}

void App::startRealTimeTimer() {
    updateTimer = new QTimer(this);
    updateTimer->callOnTimeout([this] {
        ui.label_date->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd dddd HH:mm:ss"));
    });
    updateTimer->start(1000);
}

void App::on_btn_pin_top_clicked() {
    Qt::WindowFlags flags = windowFlags();
    if (ui.btn_pin_top->isChecked()) {
        setWindowFlags(flags | Qt::WindowStaysOnTopHint);
    } else {
        setWindowFlags(flags & ~Qt::WindowStaysOnTopHint);
    }
    show();
}

void App::on_btn_add_group_clicked() {
    if (memoTreeController != nullptr) {
        memoTreeController->addGroup();
    }
}

void App::on_btn_add_record_clicked() {
    if (memoTreeController != nullptr) {
        memoTreeController->addRecordToDefaultGroup();
    }
}

void App::on_btn_hide_clicked() {
    hide();
}

void App::on_btn_close_clicked() {
    close();
}
