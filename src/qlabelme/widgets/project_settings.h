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
public:
    explicit ProjectSettings(QWidget* parent = nullptr);
    ~ProjectSettings();

    // Если позже появится редактирование - можно будет забирать список обратно
    QStringList projectClasses() const {return _classes;}
    // Передаем актуальные классы проекта в диалог
    void setProjectClasses(const QStringList& classes);

    QMap<QString, QColor> projectClassColors() const;
    void setProjectClassColors(const QMap<QString, QColor>& colors);

signals:
    void classesApplied(const QStringList& classes);
    void classColorsApplied(const QMap<QString, QColor>& colors);

private slots:
    void on_buttonBox_clicked(QAbstractButton*);
    void on_buttonBox_accepted();
    void on_buttonBox_rejected() {reject();}

private slots:
    void onSortAZ(); // Сортировать А-Я
    void onMoveUp(); // Передвинуть вверх
    void onMoveDown(); // Передвинуть вниз
    void onAddClass(); // +
    void onRemoveClass(); // -
    void onEditClass();

    void accept() override;

private:
    Q_OBJECT
    QWidget* makeClassRowWidget(QListWidgetItem* item);
    void rebuildClassRowWidget(QListWidgetItem* item);
    void syncColorsFromUi();
    void clearClassListWidgets();

private:
    Ui::ProjectSettings* ui = {nullptr};
    QStringList _classes;
    QMap<QString, QColor> _classColors;
};
