#pragma once
#include <QDialog>
#include <QStringList>
#include <QAbstractButton>

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

protected:
    void accept() override;

signals:
    void classesApplied(const QStringList& classes);

private slots:
    void on_buttonBox_clicked(QAbstractButton* btn);
    void on_buttonBox_accepted();
    void on_buttonBox_rejected() { reject(); }

    void onSortAZ(); // Сортировать А-Я
    void onMoveUp(); // Передвинуть вверх
    void onMoveDown(); // Передвинуть вниз
    void onAddClass(); // +
    void onRemoveClass(); // -
    void onEditClass();

private:
    Ui::ProjectSettings* ui = {nullptr};
    QStringList m_classes;
};
