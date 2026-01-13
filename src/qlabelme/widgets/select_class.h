#pragma once

#include <QDialog>

namespace Ui { class SelectClass; }

class SelectClass : public QDialog
{
    Q_OBJECT
public:
    explicit SelectClass(const QStringList &classes, QWidget *parent = nullptr);
    QString selectedClass() const;

private:
    Ui::SelectClass* ui = nullptr;
    QString _selectedClass;
};
