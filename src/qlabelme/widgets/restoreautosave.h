#pragma once

#include <QStringList>
#include <QDialog>

namespace Ui
{
class RestoreAutoSave;
}

class RestoreAutoSave : public QDialog
{
    Q_OBJECT

public:
    explicit RestoreAutoSave(const QStringList& documents,
                             QWidget* parent = nullptr);
    ~RestoreAutoSave() override;
    bool discardAutoSaves() const;

public slots:
    void done(int result) override;

private:
    Ui::RestoreAutoSave* ui;

    bool _discardAutoSaves = false; // Пользователь явно отказался от копий
};
