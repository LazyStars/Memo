#include "mysystemtrayicon.h"

#include <qapplication.h>
#include <qdir.h>
#include <qmenu.h>
#include <qsettings.h>

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
    QAction *action3 = new QAction(trayMenu);

    action1->setText(QString("主界面"));
    action2->setText(QString("开机自启动"));
    action2->setCheckable(true);
    action2->setChecked(isAppAutoRunEnabled());
    action3->setText(QString("退出"));

    // 添加菜单子项
    trayMenu->addAction(action1);
    trayMenu->addAction(action2);
    trayMenu->addAction(action3);

    // 显示窗体
    connect(action1, &QAction::triggered, [this](bool) {
        mParent_->activateWindow();
        mParent_->showNormal();
    });
    // 开机自启动
    connect(action2, &QAction::toggled, [this, action2](bool checked) {
        if (setAppAutoRun(checked)) {
            return;
        }

        const QSignalBlocker blocker(action2);
        action2->setChecked(!checked);
    });
    // 退出
    connect(action3, &QAction::triggered, [this](bool) {
        mParent_->close();
    });
}

bool MySystemTrayIcon::setAppAutoRun(bool isEnabled) {
#ifdef Q_OS_WIN
    QSettings settings(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
                       QSettings::NativeFormat);
    const QString appName = QApplication::applicationName();
    if (isEnabled) {
        const QString appPath = QDir::toNativeSeparators(QApplication::applicationFilePath());
        settings.setValue(appName, QStringLiteral("\"%1\"").arg(appPath));
    } else {
        settings.remove(appName);
    }
    settings.sync();
    return settings.status() == QSettings::NoError;
#else
    Q_UNUSED(isEnabled)
    return false;
#endif
}

bool MySystemTrayIcon::isAppAutoRunEnabled() const {
#ifdef Q_OS_WIN
    QSettings settings(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
                       QSettings::NativeFormat);
    return settings.contains(QApplication::applicationName());
#else
    return false;
#endif
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
