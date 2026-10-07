#pragma once

#include <QDialog>

namespace Ui {
class License;
}

class License : public QDialog
{
    Q_OBJECT

public:
    explicit License(QWidget* parent = nullptr);
    ~License() override;

private:
    Ui::License* ui;
};
