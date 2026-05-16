#pragma once

#include <QDialog>
#include <QDialogButtonBox>
#include <QCloseEvent>
#include <QStringList>

namespace Ui {

class UnsavedChanges;

}

class UnsavedChanges : public QDialog
{
    Q_OBJECT

public:
    explicit UnsavedChanges(const QStringList& documents, QWidget* parent = nullptr);
    ~UnsavedChanges() override;

    QDialogButtonBox::StandardButton selectedButton() const;

protected:
    void closeEvent(QCloseEvent* event) override;

public slots:
    void accept() override;
    void reject() override;

private:
    void loadGeometry();
    void saveGeometry() const;

private:
    Ui::UnsavedChanges* ui;
    QDialogButtonBox::StandardButton _selectedButton = QDialogButtonBox::Cancel;
};
