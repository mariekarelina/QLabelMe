#pragma once

#include <QDialog>

namespace Ui {class SelectClass;}

class SelectClass : public QDialog
{
    Q_OBJECT

public:
    explicit SelectClass(const QStringList& classes, QWidget* parent = nullptr);
    ~SelectClass();

    QString selectedClass() const;

protected:
    void closeEvent(QCloseEvent* event) override;

public slots:
    void accept() override;
    void reject() override;

private:
    void loadGeometry();
    void saveGeometry() const;

    Ui::SelectClass* ui = nullptr;
    QString _selectedClass;
};
