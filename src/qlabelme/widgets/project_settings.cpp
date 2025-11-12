#include "project_settings.h"
#include "ui_project_settings.h"

#include <QListWidgetItem>
#include <QAbstractItemView>
#include <QInputDialog>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <algorithm>

ProjectSettings::ProjectSettings(QWidget* parent)
    : QDialog(parent),
      ui(new Ui::ProjectSettings)
{
    ui->setupUi(this);
    setWindowTitle(tr("Настройки проекта"));

    if (auto b = ui->buttonBox->button(QDialogButtonBox::Ok))
        b->setText(tr("ОК"));
    if (auto b = ui->buttonBox->button(QDialogButtonBox::Apply))
        b->setText(tr("Применить"));
    if (auto b = ui->buttonBox->button(QDialogButtonBox::Cancel))
        b->setText(tr("Отмена"));

    // Всегда открываемся на первой вкладке
    if (ui->tabWidget)
        ui->tabWidget->setCurrentIndex(0);

    // Включаем ручную перестановку строк перетаскиванием
    if (ui->listClasses)
    {
        ui->listClasses->setSelectionMode(QAbstractItemView::SingleSelection);
        ui->listClasses->setDragEnabled(true);
        ui->listClasses->setAcceptDrops(true);
        ui->listClasses->setDropIndicatorShown(true);
        ui->listClasses->setDefaultDropAction(Qt::MoveAction);
        ui->listClasses->setDragDropMode(QAbstractItemView::InternalMove);
        ui->listClasses->setAlternatingRowColors(true);
    }
    if (ui->btnSortAZ)
        connect(ui->btnSortAZ, &QPushButton::clicked, this, &ProjectSettings::onSortAZ);
    if (ui->btnUp)
        connect(ui->btnUp, &QPushButton::clicked, this, &ProjectSettings::onMoveUp);
    if (ui->btnDown)
        connect(ui->btnDown, &QPushButton::clicked, this, &ProjectSettings::onMoveDown);
    if (ui->btnAdd)
        connect(ui->btnAdd, &QPushButton::clicked, this, &ProjectSettings::onAddClass);
    if (ui->btnRemove)
        connect(ui->btnRemove, &QPushButton::clicked, this, &ProjectSettings::onRemoveClass);

    if (ui->btnEdit)
        connect(ui->btnEdit,    &QPushButton::clicked, this, &ProjectSettings::onEditClass);
    // Двойной клик по элементу списка тоже редактировать
    if (ui->listClasses)
        connect(ui->listClasses, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem*) { onEditClass(); });

}

ProjectSettings::~ProjectSettings()
{
    delete ui;
}

void ProjectSettings::setProjectClasses(const QStringList& classes)
{
    m_classes = classes;
    if (ui && ui->listClasses)
    {
        ui->listClasses->clear();
        ui->listClasses->addItems(m_classes);
    }
}

void ProjectSettings::accept()
{
    // При нажатии ОК собираем актуальный порядок из виджета
    m_classes.clear();
    if (ui && ui->listClasses)
    {
        m_classes.reserve(ui->listClasses->count());
        for (int i = 0; i < ui->listClasses->count(); ++i)
        {
            m_classes << ui->listClasses->item(i)->text();
        }
    }
    QDialog::accept();
}

static QStringList collectListItems(QListWidget* lw)
{
    QStringList out;
    if (!lw)
        return out;

    out.reserve(lw->count());
    for (int i = 0; i < lw->count(); ++i)
        out << lw->item(i)->text();
    return out;
}

void ProjectSettings::on_buttonBox_clicked(QAbstractButton* btn)
{
    const auto role = ui->buttonBox->buttonRole(btn);
    if (role == QDialogButtonBox::ApplyRole) {
        m_classes = collectListItems(ui->listClasses);
        emit classesApplied(m_classes);
    }
}

void ProjectSettings::on_buttonBox_accepted()
{
    m_classes = collectListItems(ui->listClasses);
    emit classesApplied(m_classes);
    accept();
}

void ProjectSettings::onSortAZ()
{
    if (!ui || !ui->listClasses) return;

    QStringList items;
    items.reserve(ui->listClasses->count());
    for (int i = 0; i < ui->listClasses->count(); ++i)
        items << ui->listClasses->item(i)->text();

    // Сортировка без учета регистра
    std::sort(items.begin(), items.end(), [](const QString& a, const QString& b){
        return a.localeAwareCompare(b) < 0;
    });

    ui->listClasses->clear();
    ui->listClasses->addItems(items);
}

void ProjectSettings::onMoveUp()
{
    if (!ui || !ui->listClasses)
        return;
    const int row = ui->listClasses->currentRow();

    if (row <= 0)
        return;

    QListWidgetItem* it = ui->listClasses->takeItem(row);
    ui->listClasses->insertItem(row - 1, it);
    ui->listClasses->setCurrentRow(row - 1);
}

void ProjectSettings::onMoveDown()
{
    if (!ui || !ui->listClasses)
        return;
    const int row = ui->listClasses->currentRow();

    if (row < 0 || row >= ui->listClasses->count() - 1)
        return;

    QListWidgetItem* it = ui->listClasses->takeItem(row);
    ui->listClasses->insertItem(row + 1, it);
    ui->listClasses->setCurrentRow(row + 1);
}

static bool containsCaseInsensitive(const QStringList& list, const QString& s)
{
    const QString t = s.trimmed();
    for (const QString& x : list)
        if (x.compare(t, Qt::CaseInsensitive) == 0)
            return true;
    return false;
}

void ProjectSettings::onAddClass()
{
    if (!ui || !ui->listClasses) return;

    QInputDialog dlg(this);
    dlg.setModal(true);
    dlg.setInputMode(QInputDialog::TextInput);
    dlg.setWindowTitle(tr("Новый класс"));
    dlg.setLabelText(tr("Название класса:"));
    dlg.setTextValue(QString());

    dlg.setOkButtonText(tr("ОК"));
    dlg.setCancelButtonText(tr("Отмена"));

    const QSize sz(420, 160);

    // Убираем кнопки разворота/свертки
    Qt::WindowFlags f = dlg.windowFlags();
    f |= Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint;
    f &= ~Qt::WindowContextHelpButtonHint;
    f &= ~Qt::WindowMaximizeButtonHint;
    f &= ~Qt::WindowMinimizeButtonHint;
    dlg.setWindowFlags(f);

    dlg.resize(sz);
    dlg.setFixedSize(sz);
    dlg.setSizeGripEnabled(false); // Без уголка растяжения

    dlg.setWindowModality(Qt::WindowModal);
    dlg.move(this->geometry().center() - QPoint(sz.width()/2, sz.height()/2));

    if (dlg.exec() != QDialog::Accepted)
        return;

    const QString name = dlg.textValue().trimmed();
    if (name.isEmpty())
    {
        QMessageBox::information(this, tr("Добавление"), tr("Имя класса не должно быть пустым."));
        return;
    }

    // Текущие имена из виджета (чтобы проверить уникальность)
    QStringList current;
    current.reserve(ui->listClasses->count());
    for (int i = 0; i < ui->listClasses->count(); ++i)
        current << ui->listClasses->item(i)->text();

    if (containsCaseInsensitive(current, name))
    {
        QMessageBox::information(this, tr("Добавление"),
                                 tr("Класс с таким именем уже существует."));
        return;
    }

    auto exists = std::any_of(current.begin(), current.end(),
                              [&](const QString& s){ return s.compare(name, Qt::CaseInsensitive) == 0; });
    if (exists) return;

    int row = ui->listClasses->currentRow();
    if (row < 0) row = ui->listClasses->count() - 1;
    // Добавляем после текущей строки
    QListWidgetItem* it = new QListWidgetItem(name);
    ui->listClasses->insertItem(row + 1, it);
    ui->listClasses->setCurrentRow(row + 1);
}

void ProjectSettings::onRemoveClass()
{
    if (!ui || !ui->listClasses) return;

    const int row = ui->listClasses->currentRow();
    if (row < 0) return;

    const QString name = ui->listClasses->item(row)->text();

    QMessageBox box(this);
    box.setIcon(QMessageBox::Question);
    box.setWindowTitle(tr("Удаление класса"));
    box.setText(tr("Удалить класс «%1» из списка?").arg(name));
    box.setStandardButtons(QMessageBox::Yes | QMessageBox::No);

    if (auto b = box.button(QMessageBox::Yes)) b->setText(tr("Да"));
    if (auto b = box.button(QMessageBox::No))  b->setText(tr("Нет"));

    const QSize sz(380, 160);
    box.resize(sz);
    box.setFixedSize(sz);
    box.setSizeGripEnabled(false);

    Qt::WindowFlags f = box.windowFlags();
    f |= Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint;
    f &= ~Qt::WindowContextHelpButtonHint;
    f &= ~Qt::WindowMaximizeButtonHint;
    f &= ~Qt::WindowMinimizeButtonHint;
    box.setWindowFlags(f);

    box.setWindowModality(Qt::WindowModal);
    box.move(this->geometry().center() - QPoint(sz.width()/2, sz.height()/2));

    if (box.exec() != QMessageBox::Yes)
        return;

    delete ui->listClasses->takeItem(row);
    const int newRow = qMin(row, ui->listClasses->count() - 1);
    if (newRow >= 0)
        ui->listClasses->setCurrentRow(newRow);
}

static bool containsCaseInsensitiveExcept(const QStringList& list, const QString& candidate, const QString& except)
{
    const QString c = candidate.trimmed();
    for (const QString& x : list) {
        if (x.compare(except, Qt::CaseInsensitive) == 0) continue;
        if (x.compare(c, Qt::CaseInsensitive) == 0) return true;
    }
    return false;
}

void ProjectSettings::onEditClass()
{
    if (!ui || !ui->listClasses) return;

    const int row = ui->listClasses->currentRow();
    if (row < 0) return;

    const QString oldName = ui->listClasses->item(row)->text();

    // Соберем текущие имена
    QStringList current;
    current.reserve(ui->listClasses->count());
    for (int i = 0; i < ui->listClasses->count(); ++i)
        current << ui->listClasses->item(i)->text();

    QInputDialog dlg(this);
    dlg.setModal(true);
    dlg.setInputMode(QInputDialog::TextInput);
    dlg.setWindowTitle(tr("Редактировать класс"));
    dlg.setLabelText(tr("Новое имя для «%1»:").arg(oldName));
    dlg.setTextValue(oldName);
    dlg.setOkButtonText(tr("ОК"));
    dlg.setCancelButtonText(tr("Отмена"));

    const QSize sz(420, 160);

    Qt::WindowFlags f = dlg.windowFlags();
    f |= Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint;
    f &= ~Qt::WindowContextHelpButtonHint;
    f &= ~Qt::WindowMaximizeButtonHint;
    f &= ~Qt::WindowMinimizeButtonHint;
    dlg.setWindowFlags(f);

    dlg.resize(sz);
    dlg.setMinimumSize(sz);
    dlg.setMaximumSize(sz);
    dlg.setFixedSize(sz);
    dlg.setSizeGripEnabled(false);
    dlg.setWindowModality(Qt::WindowModal);
    dlg.move(this->geometry().center() - QPoint(sz.width()/2, sz.height()/2));

    if (dlg.exec() != QDialog::Accepted)
        return;

    const QString newName = dlg.textValue().trimmed();

    if (newName.isEmpty())
    {
        QMessageBox::information(this, tr("Редактирование"), tr("Имя класса не должно быть пустым."));
        return;
    }
    if (newName.compare(oldName, Qt::CaseInsensitive) == 0)
    {
        return;
    }
    if (containsCaseInsensitiveExcept(current, newName, oldName))
    {
        QMessageBox::information(this, tr("Редактирование"),
                                 tr("Класс с таким именем уже существует."));
        return;
    }

    ui->listClasses->item(row)->setText(newName);
    ui->listClasses->setCurrentRow(row);
}
