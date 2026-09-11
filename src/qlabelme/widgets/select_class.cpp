#include "select_class.h"
#include "ui_select_class.h"
#include "load_geometry.h"

#include "shared/config/appl_conf.h"

#include <QCloseEvent>

SelectClass::SelectClass(const QStringList &classes, QWidget* parent)
    : QDialog(parent),
      ui(new Ui::SelectClass)
{
    ui->setupUi(this);

    // Убираем кнопки свернуть/развернуть/закрыть в заголовке
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowFlags(windowFlags() & ~Qt::WindowMinMaxButtonsHint);

    // Запрещаем изменение размера окна
    setFixedSize(400, 500);
    loadGeometry();

    // Заполняем список классами
    ui->listWidget->clear();
    ui->listWidget->addItems(classes);
    if (!classes.isEmpty())
        ui->listWidget->setCurrentRow(0);

    // Выбор по клику сразу закрываем диалог
    connect(ui->listWidget, &QListWidget::itemClicked, this,
            [this](QListWidgetItem* item) {
                _selectedClass = item ? item->text() : QString();
                accept();
            });

    // Двойной щелчок тоже подтверждает
    connect(ui->listWidget, &QListWidget::itemDoubleClicked, this,
            [this](QListWidgetItem* item) {
                _selectedClass = item ? item->text() : QString();
                accept();
            });
}

SelectClass::~SelectClass()
{
    delete ui;
}

QString SelectClass::selectedClass() const
{
    return _selectedClass;
}

void SelectClass::closeEvent(QCloseEvent* event)
{
    saveGeometry();
    QDialog::closeEvent(event);
}

void SelectClass::accept()
{
    saveGeometry();
    QDialog::accept();
}

void SelectClass::reject()
{
    saveGeometry();
    QDialog::reject();
}

void SelectClass::loadGeometry()
{
    dialogLoadGeometry(config::base(),
                       "windows.select_class.geometry",
                       this);
}

void SelectClass::saveGeometry() const
{
    const QRect geometry = this->geometry();

    const QVector<int> windowGeometry {
        geometry.x(),
        geometry.y(),
        geometry.width(),
        geometry.height()
    };

    config::base().setValue("windows.select_class.geometry",
                            windowGeometry);
}
