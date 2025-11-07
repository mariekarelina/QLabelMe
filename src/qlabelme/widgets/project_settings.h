#pragma once
#include <QDialog>
#include <QStringList>

namespace Ui { class ProjectSettings; }

class ProjectSettings : public QDialog
{
    Q_OBJECT
public:
    explicit ProjectSettings(QWidget* parent = nullptr);
    ~ProjectSettings();

    // Передаем актуальные классы проекта в диалог
    void setProjectClasses(const QStringList& classes);
    // Если позже появится редактирование - можно будет забирать список обратно
    QStringList projectClasses() const { return m_classes; }

private:
    Ui::ProjectSettings* ui;
    QStringList m_classes;
};
