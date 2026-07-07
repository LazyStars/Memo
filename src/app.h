#pragma once

#include "ui_app.h"

class App : public QWidget
{
    Q_OBJECT

public:
    explicit App(QWidget* parent = nullptr);

private:
    Ui::App ui;
};