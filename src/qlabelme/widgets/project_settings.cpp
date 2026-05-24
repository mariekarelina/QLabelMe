#include "project_settings.h"
#include "ui_project_settings.h"

#include "shared/defmac.h"
#include "message_box.h"

#include <QListWidgetItem>
#include <QAbstractItemView>
#include <QInputDialog>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QColor>
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QColorDialog>
#include <algorithm>

static constexpr int RoleClassName = Qt::UserRole + 1; // Имя класса для QLabel

static void applyColorToButton(QToolButton* btn, const QColor& c)
{
    if (!btn) return;

    btn->setStyleSheet(QString(
        "QToolButton {"
        "  border: 1px solid #777;"
        "  background: %1;"
        "  min-width: 16px; max-width: 16px;"
        "  min-height: 16px; max-height: 16px;"
        "  border-radius: 2px;"
        "}"
    ).arg(c.name(QColor::HexRgb)));
}

ProjectSettings::ProjectSettings(QWidget* parent)
    : QDialog(parent),
      ui(new Ui::ProjectSettings)
{
    ui->setupUi(this);
    setWindowTitle(u8"Настройки проекта");

    if (QPushButton* b = ui->buttonBox->button(QDialogButtonBox::Ok))
        b->setText(u8"ОК");

    if (QPushButton* b = ui->buttonBox->button(QDialogButtonBox::Apply))
        b->setText(u8"Применить");

    if (QPushButton* b = ui->buttonBox->button(QDialogButtonBox::Cancel))
        b->setText(u8"Отмена");

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
        ui->listClasses->setStyleSheet(
            "QListWidget::item { color: transparent; }"
            "QListWidget::item:selected { color: transparent; }"
        );
    }
    if (ui->btnSortAZ)
        chk_connect_a(ui->btnSortAZ, &QPushButton::clicked, this, &ProjectSettings::onSortAZ);
    if (ui->btnUp)
        chk_connect_a(ui->btnUp, &QPushButton::clicked, this, &ProjectSettings::onMoveUp);
    if (ui->btnDown)
        chk_connect_a(ui->btnDown, &QPushButton::clicked, this, &ProjectSettings::onMoveDown);
    if (ui->btnAdd)
        chk_connect_a(ui->btnAdd, &QPushButton::clicked, this, &ProjectSettings::onAddClass);
    if (ui->btnRemove)
        chk_connect_a(ui->btnRemove, &QPushButton::clicked, this, &ProjectSettings::onRemoveClass);

    if (ui->btnEdit)
        chk_connect_a(ui->btnEdit,    &QPushButton::clicked, this, &ProjectSettings::onEditClass);
    // Двойной клик по элементу списка тоже редактировать
    if (ui->listClasses)
        chk_connect_a(ui->listClasses, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem*) { onEditClass(); });
    // Иконки выбора цвета класса
    ui->listClasses->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->listClasses->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->listClasses->viewport()->setMouseTracking(true);
}

ProjectSettings::~ProjectSettings()
{
    delete ui;
}

void ProjectSettings::setProjectClasses(const QStringList& classes)
{
    _classes = classes;
    if (ui && ui->listClasses)
    {
        clearClassListWidgets();
        for (int i = 0; i < _classes.size(); ++i)
        {
            const QString& name = _classes[i];

            QListWidgetItem* it = new QListWidgetItem();
            it->setText("");
            it->setData(RoleClassName, name);

            QColor c = _classColors.value(name, QColor());

            it->setData(Qt::UserRole, c);

            ui->listClasses->addItem(it);
            rebuildClassRowWidget(it);
        }
    }
}

QMap<QString, QColor> ProjectSettings::projectClassColors() const
{
    QMap<QString, QColor> out;
    if (!ui || !ui->listClasses)
        return out;

    for (int i = 0; i < ui->listClasses->count(); ++i)
    {
        QListWidgetItem* it = ui->listClasses->item(i);
        const QString name = it->data(RoleClassName).toString();
        const QColor c = it->data(Qt::UserRole).value<QColor>();
        if (!name.isEmpty() && c.isValid())
            out[name] = c;
    }
    return out;
}

void ProjectSettings::setProjectClassColors(const QMap<QString, QColor>& colors)
{
    _classColors = colors;

    if (!ui || !ui->listClasses)
        return;

    for (int i = 0; i < ui->listClasses->count(); ++i)
    {
        QListWidgetItem* it = ui->listClasses->item(i);
        const QString name = it->data(RoleClassName).toString();
        const QColor c = colors.value(name, QColor());
        if (c.isValid())
        {
            it->setData(Qt::UserRole, c);
            rebuildClassRowWidget(it);
        }
    }
}

void ProjectSettings::accept()
{
    // При нажатии ОК собираем актуальный порядок из виджета
    _classes.clear();
    if (ui && ui->listClasses)
    {
        _classes.reserve(ui->listClasses->count());
        for (int i = 0; i < ui->listClasses->count(); ++i)
        {
            //_classes << ui->listClasses->item(i)->text();
            _classes << ui->listClasses->item(i)->data(RoleClassName).toString();
        }
    }
    syncColorsFromUi();
    QDialog::accept();
}

static QStringList collectListItems(QListWidget* lw)
{
    QStringList out;
    if (!lw)
        return out;

    out.reserve(lw->count());
    for (int i = 0; i < lw->count(); ++i)
        //out << lw->item(i)->text();
        out << lw->item(i)->data(RoleClassName).toString();
    return out;
}

void ProjectSettings::on_buttonBox_clicked(QAbstractButton* btn)
{
    const QDialogButtonBox::ButtonRole role = ui->buttonBox->buttonRole(btn);
    if (role == QDialogButtonBox::ApplyRole)
    {
        _classes = collectListItems(ui->listClasses);
        syncColorsFromUi();

        emit classesApplied(_classes);
        emit classColorsApplied(_classColors);
    }
}

void ProjectSettings::on_buttonBox_accepted()
{
    _classes = collectListItems(ui->listClasses);
    syncColorsFromUi();

    emit classesApplied(_classes);
    emit classColorsApplied(_classColors);

    accept();
}

void ProjectSettings::onSortAZ()
{
    if (!ui || !ui->listClasses)
        return;

    struct ItemData
    {
        QString name;
        QColor  color;
    };

    QVector<ItemData> items;
    items.reserve(ui->listClasses->count());

    for (int i = 0; i < ui->listClasses->count(); ++i)
    {
        QListWidgetItem* it = ui->listClasses->item(i);
        ItemData d;
        //d.name = it->text();
        d.name = it->data(RoleClassName).toString();

        d.color = it->data(Qt::UserRole).value<QColor>();
        items.append(d);
    }

    std::sort(items.begin(), items.end(), [](const ItemData& a, const ItemData& b){
        return a.name.localeAwareCompare(b.name) < 0;
    });

    clearClassListWidgets();

    for (int i = 0; i < items.size(); ++i)
    {
        QListWidgetItem* it = new QListWidgetItem();
        it->setText("");
        it->setData(RoleClassName, items[i].name);
        it->setData(Qt::UserRole, items[i].color);
        ui->listClasses->addItem(it);
        rebuildClassRowWidget(it);
    }

    if (ui->listClasses->count() > 0)
        ui->listClasses->setCurrentRow(0);
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

    rebuildClassRowWidget(it);

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

    rebuildClassRowWidget(it);

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
    dlg.setWindowTitle(u8"Новый класс");
    dlg.setLabelText(u8"Название класса:");
    dlg.setTextValue(QString());

    dlg.setOkButtonText(u8"ОК");
    dlg.setCancelButtonText(u8"Отмена");

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
    //dlg.move(this->geometry().center() - QPoint(sz.width()/2, sz.height()/2));
    const QPoint centerGlobal = this->mapToGlobal(this->rect().center());
    dlg.move(centerGlobal - QPoint(sz.width()/2, sz.height()/2));

    if (dlg.exec() != QDialog::Accepted)
        return;

    const QString name = dlg.textValue().trimmed();
    if (name.isEmpty())
    {
        messageBox(
            this,
            QMessageBox::Information,
            u8"Имя класса не должно быть пустым"
        );
        return;
    }

    // Текущие имена из виджета (чтобы проверить уникальность)
    QStringList current;
    current.reserve(ui->listClasses->count());
    for (int i = 0; i < ui->listClasses->count(); ++i)
        //current << ui->listClasses->item(i)->text();
        current << ui->listClasses->item(i)->data(RoleClassName).toString();

    if (containsCaseInsensitive(current, name))
    {
        messageBox(
            this,
            QMessageBox::Information,
            u8"Класс с таким именем уже существует"
        );
        return;
    }

    const bool exists = std::any_of(current.begin(), current.end(),
                                    [&](const QString& s)
                                    {
                                        return s.compare(name, Qt::CaseInsensitive) == 0;
                                    });
    if (exists) return;

    const int row = ui->listClasses->currentRow();
    const int insertRow = (row < 0) ? ui->listClasses->count() : (row + 1);

    QListWidgetItem* it = new QListWidgetItem();
    it->setText("");
    it->setData(RoleClassName, name);

    it->setData(Qt::UserRole, QColor());

    ui->listClasses->insertItem(insertRow, it);
    rebuildClassRowWidget(it);
    ui->listClasses->setCurrentItem(it);
}

void ProjectSettings::onRemoveClass()
{
    if (!ui || !ui->listClasses) return;

    const int row = ui->listClasses->currentRow();
    if (row < 0) return;

    const QString name = ui->listClasses->item(row)->data(RoleClassName).toString();

    const QMessageBox::StandardButton answer = messageBox(
        this,
        QMessageBox::Question,
        QString::fromUtf8("Удалить класс «%1» из списка?").arg(name),
        0,
        [](QMessageBox* box)
        {
            box->setWindowTitle(u8"Удаление класса");
            box->setStandardButtons(QMessageBox::Yes | QMessageBox::No);

            if (QAbstractButton* button = box->button(QMessageBox::Yes))
                button->setText(u8"Да");

            if (QAbstractButton* button = box->button(QMessageBox::No))
                button->setText(u8"Нет");
        }
    );

    if (answer != QMessageBox::Yes)
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

    //const QString oldName = ui->listClasses->item(row)->text();
    const QString oldName = ui->listClasses->item(row)->data(RoleClassName).toString();


    // Соберем текущие имена
    QStringList current;
    current.reserve(ui->listClasses->count());
    for (int i = 0; i < ui->listClasses->count(); ++i)
        //current << ui->listClasses->item(i)->text();
        current << ui->listClasses->item(i)->data(RoleClassName).toString();

    QInputDialog dlg(this);
    dlg.setModal(true);
    dlg.setInputMode(QInputDialog::TextInput);
    dlg.setWindowTitle(u8"Редактировать класс");
    dlg.setLabelText(QString::fromUtf8("Новое имя для «%1»:").arg(oldName));
    dlg.setTextValue(oldName);
    dlg.setOkButtonText(u8"ОК");
    dlg.setCancelButtonText(u8"Отмена");

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
        messageBox(
            this,
            QMessageBox::Information,
            u8"Имя класса не должно быть пустым"
        );
        return;
    }
    if (newName.compare(oldName, Qt::CaseInsensitive) == 0)
    {
        return;
    }
    if (containsCaseInsensitiveExcept(current, newName, oldName))
    {
        messageBox(
            this,
            QMessageBox::Information,
            u8"Класс с таким именем уже существует"
        );
        return;
    }

    // ui->listClasses->item(row)->setText(newName);
    // ui->listClasses->setCurrentRow(row);
    QListWidgetItem* it = ui->listClasses->item(row);
    //QColor c = it->data(Qt::UserRole).value<QColor>();
    it->setData(RoleClassName, newName);
    rebuildClassRowWidget(it);

    ui->listClasses->setCurrentRow(row);
}

// Создаем виджет-строку
QWidget* ProjectSettings::makeClassRowWidget(QListWidgetItem* item)
{
    QWidget* w = new QWidget(ui->listClasses);
    //w->setMinimumHeight(36);
    //item->setSizeHint(QSize(0, 36));

    QHBoxLayout* lay = new QHBoxLayout(w);
    //lay->setContentsMargins(6, 0, 6, 0);
    lay->setSpacing(6);
    lay->setContentsMargins(6, 4, 6, 4);

    QLabel* lbl = new QLabel(item->data(RoleClassName).toString(), w);

    w->setStyleSheet("background: transparent;");
    lbl->setStyleSheet("background: transparent;");

    lbl->setAttribute(Qt::WA_TransparentForMouseEvents, true);

    QToolButton* btn = new QToolButton(w);
    btn->setAutoRaise(true);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setFocusPolicy(Qt::NoFocus);

    // Центрируем содержимое QLabel
    lbl->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

    // Запрещаем кнопке растягиваться и фиксируем размер
    btn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    btn->setFixedSize(16, 16);

    // Выравниваем элементы в layout по вертикальному центру
    lay->setAlignment(lbl, Qt::AlignVCenter);
    lay->setAlignment(btn, Qt::AlignVCenter);

    QColor c = item->data(Qt::UserRole).value<QColor>();
    applyColorToButton(btn, c.isValid() ? c : QColor("#808080"));

    lay->addWidget(lbl);
    lay->addStretch(1);
    lay->addWidget(btn);

    const QFontMetrics fm(lbl->font());

    const int marginsV =
        lay->contentsMargins().top() + lay->contentsMargins().bottom();

    const int safeFontHeight = fm.height() + fm.leading();

    const int rowH = qMax(safeFontHeight, btn->sizeHint().height())
                     + marginsV;

    w->setMinimumHeight(rowH);
    w->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    item->setSizeHint(w->sizeHint());

    connect(btn, &QToolButton::clicked, this, [this, item, btn]()
    {
        QColor cur = item->data(Qt::UserRole).value<QColor>();
        if (!cur.isValid())
            cur = QColor("#3b6cff");

        QColor chosen = QColorDialog::getColor(cur, this, u8"Цвет");
        if (!chosen.isValid())
            return;

        item->setData(Qt::UserRole, chosen);
        applyColorToButton(btn, chosen);
    });

    w->setProperty("classLabelPtr", QVariant::fromValue<void*>(lbl));
    w->setProperty("classColorBtnPtr", QVariant::fromValue<void*>(btn));

    w->setProperty("classRow", true);
    lbl->setProperty("classRow", true);

    return w;
}

void ProjectSettings::rebuildClassRowWidget(QListWidgetItem* item)
{
    if (!item || !ui->listClasses) return;

    // Удалим старый виджет, если был
    if (QWidget* old = ui->listClasses->itemWidget(item))
    {
        ui->listClasses->setItemWidget(item, nullptr);
        delete old;
    }

    // Высота строки
    //item->setSizeHint(QSize(0, 22));
    ui->listClasses->setItemWidget(item, makeClassRowWidget(item));
}

void ProjectSettings::syncColorsFromUi()
{
    _classColors.clear();
    if (!ui || !ui->listClasses)
        return;

    for (int i = 0; i < ui->listClasses->count(); ++i)
    {
        QListWidgetItem* it = ui->listClasses->item(i);
        const QString name = it->data(RoleClassName).toString();
        const QColor  c    = it->data(Qt::UserRole).value<QColor>();

        if (!name.isEmpty() && c.isValid())
            _classColors[name] = c;
    }
}

void ProjectSettings::clearClassListWidgets()
{
    if (!ui || !ui->listClasses) return;

    // Удаляем виджеты строк, которые были установлены через setItemWidget
    for (int i = 0; i < ui->listClasses->count(); ++i)
    {
        QListWidgetItem* it = ui->listClasses->item(i);
        if (QWidget* w = ui->listClasses->itemWidget(it))
        {
            ui->listClasses->setItemWidget(it, nullptr);
            delete w;
        }
    }
    ui->listClasses->clear();
}
