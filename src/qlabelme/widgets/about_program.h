#pragma once

#include <QDialog>

namespace Ui {
class AboutProgram;
}

class AboutProgram : public QDialog
{
    Q_OBJECT

public:
    explicit AboutProgram(QWidget* parent = nullptr);
    ~AboutProgram() override;

protected:
    void closeEvent(QCloseEvent* event) override;

public slots:
    void accept() override;
    void reject() override;

private:
    void loadGeometry();
    void saveGeometry() const;

private:
    Ui::AboutProgram* ui;
};
