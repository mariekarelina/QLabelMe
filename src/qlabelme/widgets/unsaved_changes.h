#pragma once

#include <QDialogButtonBox>
#include <QCloseEvent>
#include <QStringList>
#include <QDialog>
#include <QSet>

namespace Ui {

class UnsavedChanges;

}

class UnsavedChanges : public QDialog
{
public:
    explicit UnsavedChanges(const QStringList& documents, QWidget* parent = nullptr);
    ~UnsavedChanges() override;

    QDialogButtonBox::StandardButton selectedButton() const;

    void loadGeometry();
    void saveGeometry() const;

    QSet<QString> checkedDocumentPaths() const;

public slots:
    void accept() override;
    void reject() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    Q_OBJECT
    Ui::UnsavedChanges* ui;
    QDialogButtonBox::StandardButton _selectedButton = QDialogButtonBox::Cancel;
};
