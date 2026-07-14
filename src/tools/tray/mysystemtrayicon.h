#pragma once

#include <qsystemtrayicon.h>

class MySystemTrayIcon : public QSystemTrayIcon {
    Q_OBJECT

public:
    explicit MySystemTrayIcon(QWidget *parent);
    ~MySystemTrayIcon() override;

private:
    void createTrayMenu();
    bool setAppAutoRun(bool isEnabled);
    bool isAppAutoRunEnabled() const;

private slots:
    void onIconActivated(QSystemTrayIcon::ActivationReason reason);

private:
    QWidget *mParent_;
    QMenu *trayMenu;
};
