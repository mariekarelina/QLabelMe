#pragma once

#include <QDialog>

namespace Ui { class Select_class; }

class Select_class : public QDialog
{
    Q_OBJECT
public:
    explicit Select_class(const QStringList &classes, QWidget *parent = nullptr);
    QString selectedClass() const;

private:
    Ui::Select_class* ui = nullptr;
    QString _selectedClass;
};
