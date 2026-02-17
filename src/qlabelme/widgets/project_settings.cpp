#include "project_settings.h"
#include "ui_project_settings.h"

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

// Выбор цвета
static QColor defaultColorForIndex(int i)
{
    QColor c;
    c.setHsl((i * 47) % 360, 200, 120);
    return c;
}

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

// static QColor normalizeColor(const QColor& c, const QColor& fallback)
// {
//     if (!c.isValid())
//         return fallback;
//     return c;
// }

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
        ui->listClasses->setStyleSheet(
            "QListWidget::item { color: transparent; }"
            "QListWidget::item:selected { color: transparent; }"
        );
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
    m_classes = classes;
    if (ui && ui->listClasses)
    {
        clearClassListWidgets();
        for (int i = 0; i < m_classes.size(); ++i)
        {
            const QString& name = m_classes[i];

            auto* it = new QListWidgetItem();
            it->setText("");
            it->setData(RoleClassName, name);

            QColor c = m_classColors.value(name, QColor());
            if (!c.isValid())
                c = defaultColorForIndex(i);

            it->setData(Qt::UserRole, c);

            ui->listClasses->addItem(it);
            rebuildClassRowWidget(it);
        }
    }
}

void ProjectSettings::setProjectClassColors(const QMap<QString, QColor>& colors)
{
    if (!ui || !ui->listClasses)
        return;

    for (int i = 0; i < ui->listClasses->count(); ++i)
    {
        auto* it = ui->listClasses->item(i);
        const QString name = it->data(RoleClassName).toString();
        const QColor c = colors.value(name, QColor());
        if (c.isValid())
        {
            it->setData(Qt::UserRole, c);
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
        auto* it = ui->listClasses->item(i);
        const QString name = it->data(RoleClassName).toString();
        const QColor c = it->data(Qt::UserRole).value<QColor>();
        if (!name.isEmpty() && c.isValid())
            out[name] = c;
    }
    return out;
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
            //m_classes << ui->listClasses->item(i)->text();
            m_classes << ui->listClasses->item(i)->data(RoleClassName).toString();
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
    const auto role = ui->buttonBox->buttonRole(btn);
    if (role == QDialogButtonBox::ApplyRole)
    {
        m_classes = collectListItems(ui->listClasses);
        syncColorsFromUi();

        emit classesApplied(m_classes);
        emit classColorsApplied(m_classColors);
    }
}

void ProjectSettings::on_buttonBox_accepted()
{
    m_classes = collectListItems(ui->listClasses);
    syncColorsFromUi();

    emit classesApplied(m_classes);
    emit classColorsApplied(m_classColors);

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
        auto* it = ui->listClasses->item(i);
        ItemData d;
        //d.name = it->text();
        d.name = it->data(RoleClassName).toString();

        d.color = it->data(Qt::UserRole).value<QColor>();
        if (!d.color.isValid())
            d.color = defaultColorForIndex(i);
        items.push_back(d);
    }

    std::sort(items.begin(), items.end(), [](const ItemData& a, const ItemData& b){
        return a.name.localeAwareCompare(b.name) < 0;
    });

    clearClassListWidgets();

    for (int i = 0; i < items.size(); ++i)
    {
        auto* it = new QListWidgetItem();
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
    //dlg.move(this->geometry().center() - QPoint(sz.width()/2, sz.height()/2));
    const QPoint centerGlobal = this->mapToGlobal(this->rect().center());
    dlg.move(centerGlobal - QPoint(sz.width()/2, sz.height()/2));

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
        //current << ui->listClasses->item(i)->text();
        current << ui->listClasses->item(i)->data(RoleClassName).toString();

    if (containsCaseInsensitive(current, name))
    {
        QMessageBox::information(this, tr("Добавление"),
                                 tr("Класс с таким именем уже существует."));
        return;
    }

    auto exists = std::any_of(current.begin(), current.end(),
                              [&](const QString& s){ return s.compare(name, Qt::CaseInsensitive) == 0; });
    if (exists) return;

    const int row = ui->listClasses->currentRow();
    const int insertRow = (row < 0) ? ui->listClasses->count() : (row + 1);

    auto* it = new QListWidgetItem();
    it->setText("");
    it->setData(RoleClassName, name);

    QColor color = defaultColorForIndex(ui->listClasses->count());
    it->setData(Qt::UserRole, color);

    ui->listClasses->insertItem(insertRow, it);
    rebuildClassRowWidget(it);
    ui->listClasses->setCurrentItem(it);
}

void ProjectSettings::onRemoveClass()
{
    if (!ui || !ui->listClasses) return;

    const int row = ui->listClasses->currentRow();
    if (row < 0) return;

    //const QString name = ui->listClasses->item(row)->text();
    const QString name = ui->listClasses->item(row)->data(RoleClassName).toString();


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
    auto* w = new QWidget(ui->listClasses);
    //w->setMinimumHeight(36);
    //item->setSizeHint(QSize(0, 36));

    auto* lay = new QHBoxLayout(w);
    //lay->setContentsMargins(6, 0, 6, 0);
    lay->setSpacing(6);
    lay->setContentsMargins(6, 4, 6, 4);

    auto* lbl = new QLabel(item->data(RoleClassName).toString(), w);

    w->setStyleSheet("background: transparent;");
    lbl->setStyleSheet("background: transparent;");

    lbl->setAttribute(Qt::WA_TransparentForMouseEvents, true);

    auto* btn = new QToolButton(w);
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
    if (!c.isValid())
        c = QColor("#3b6cff");
    applyColorToButton(btn, c);

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
        if (!cur.isValid()) cur = QColor("#3b6cff");

        QColor chosen = QColorDialog::getColor(cur, this, tr("Цвет"));
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
    m_classColors.clear();
    if (!ui || !ui->listClasses)
        return;

    for (int i = 0; i < ui->listClasses->count(); ++i)
    {
        QListWidgetItem* it = ui->listClasses->item(i);
        const QString name = it->data(RoleClassName).toString();
        const QColor  c    = it->data(Qt::UserRole).value<QColor>();

        if (!name.isEmpty() && c.isValid())
            m_classColors[name] = c;
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
