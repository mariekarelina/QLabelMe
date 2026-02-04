#pragma once
#include <QDialog>
#include <QStringList>
#include <QAbstractButton>
#include <QListWidgetItem>
#include <QMap>
#include <QColor>

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
    void setProjectClassColors(const QMap<QString, QColor>& colors);
    QMap<QString, QColor> projectClassColors() const;

protected:
    void accept() override;

signals:
    void classesApplied(const QStringList& classes);
    void classColorsApplied(const QMap<QString, QColor>& colors);

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
    QWidget* makeClassRowWidget(QListWidgetItem* item);
    void rebuildClassRowWidget(QListWidgetItem* item);
    void syncColorsFromUi();
    void clearClassListWidgets();

private:
    Ui::ProjectSettings* ui = {nullptr};
    QStringList m_classes;
    QMap<QString, QColor> m_classColors;
};
