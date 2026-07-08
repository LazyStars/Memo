#include "mysystemtrayicon.h"

#include <qmenu.h>

MySystemTrayIcon::MySystemTrayIcon(QWidget *parent)
    : QSystemTrayIcon(parent)
    , mParent_(parent)
{
    setToolTip(tr("备忘录"));
    setIcon(QIcon(":/res/logo.ico"));
    createTrayMenu();
    connect(this, &MySystemTrayIcon::activated, this, &MySystemTrayIcon::onIconActivated);

    show();
}

MySystemTrayIcon::~MySystemTrayIcon() {
    mParent_->deleteLater();
}

void MySystemTrayIcon::createTrayMenu() {
    trayMenu = new QMenu(mParent_);
    QAction *action1 = new QAction(trayMenu);
    QAction *action2 = new QAction(trayMenu);

    action1->setText(QString("主界面"));
    action2->setText(QString("退出"));

    // 添加菜单子项
    trayMenu->addAction(action1);
    trayMenu->addAction(action2);

    // 显示窗体
    connect(action1, &QAction::triggered, [this](bool) {
        mParent_->activateWindow();
        mParent_->showNormal();
    });
    // 退出
    connect(action2, &QAction::triggered, [this](bool) {
        mParent_->close();
    });
}

void MySystemTrayIcon::onIconActivated(QSystemTrayIcon::ActivationReason reason) {
    switch (reason) {
        case ActivationReason::Trigger:
        case ActivationReason::DoubleClick:
            mParent_->show();
            break;
        case QSystemTrayIcon::Context:
            trayMenu->exec(QCursor::pos());
            break;
        default:
            break;
    }
}
