#include "select_class.h"
#include "ui_select_class.h"


Select_class::Select_class(const QStringList &classes, QWidget *parent)
    : QDialog(parent),
      ui(new Ui::Select_class)
{
    ui->setupUi(this);

    // Убираем кнопки свернуть/развернуть/закрыть в заголовке
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowFlags(windowFlags() & ~Qt::WindowMinMaxButtonsHint);

    // Запрещаем изменение размера окна
    setFixedSize(400, 500);

    // Заполняем список классами
    ui->_listWidget->clear();
    ui->_listWidget->addItems(classes);
    if (!classes.isEmpty())
        ui->_listWidget->setCurrentRow(0);

    // Выбор по клику сразу закрываем диалог
    connect(ui->_listWidget, &QListWidget::itemClicked, this,
            [this](QListWidgetItem* item) {
                _selectedClass = item ? item->text() : QString();
                accept();
            });

    // Двойной щелчок тоже подтверждает
    connect(ui->_listWidget, &QListWidget::itemDoubleClicked, this,
            [this](QListWidgetItem* item) {
                _selectedClass = item ? item->text() : QString();
                accept();
            });
}

QString Select_class::selectedClass() const
{
    return _selectedClass;
}
