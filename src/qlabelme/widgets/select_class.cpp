#include "select_class.h"

Selection_class::Selection_class(const QStringList &classes, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Выберите класс для фигуры");

    // Убираем кнопки свернуть/развернуть/закрыть в заголовке
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowFlags(windowFlags() & ~Qt::WindowMinMaxButtonsHint);

    // Запрещаем изменение размера окна
    setFixedSize(300, 400);

    _listWidget = new QListWidget(this);
    _listWidget->addItems(classes);
    _listWidget->setSelectionMode(QListWidget::SingleSelection);

    //buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    //connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    //connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // Закрываем диалог сразу при выборе элемента
    connect(_listWidget, &QListWidget::itemClicked, [this](QListWidgetItem* item) {
        _selectedClass = item->text();
        accept(); // Закрываем диалог с результатом Accepted
    });

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(_listWidget);
    //layout->addWidget(buttonBox);
}

QString Selection_class::selectedClass() const
{
    return _selectedClass;
}
