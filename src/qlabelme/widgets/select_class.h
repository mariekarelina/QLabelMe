#pragma once

#include <QDialog>
#include <QListWidget>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QListWidgetItem>

class Selection_class : public QDialog
{
    Q_OBJECT
public:
    explicit Selection_class(const QStringList &classes, QWidget *parent = nullptr);
    QString selectedClass() const;

private:
    QListWidget* _listWidget;
    QDialogButtonBox* _buttonBox;
    QString _selectedClass;
};
