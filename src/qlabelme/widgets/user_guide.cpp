#include "user_guide.h"
#include "ui_user_guide.h"

#include <QTreeWidgetItem>
#include <QPushButton>


UserGuide::UserGuide(QWidget *parent)
    : QDialog(parent),
      ui(new Ui::UserGuide)
{
    ui->setupUi(this);

    setWindowTitle(tr("Руководство пользователя QLabelMe"));
    resize(900, 600);
    setAttribute(Qt::WA_DeleteOnClose);

    buildContents();

    connect(ui->treeContents, &QTreeWidget::itemSelectionChanged, this, [this]()
    {
        auto items = ui->treeContents->selectedItems();
        if (items.isEmpty())
            return;

        const QString key = items.first()->data(0, Qt::UserRole).toString();
        showPage(key);
    });

    // Показать первую страницу сразу
    showPage("intro");

    ui->textBrowser->setOpenExternalLinks(true);
    ui->textBrowser->setStyleSheet("QTextBrowser { padding: 8px; }");

    connect(ui->buttonBox, &QDialogButtonBox::rejected,
        this, &QDialog::reject);
    ui->buttonBox->button(QDialogButtonBox::Close)->setText(tr("Закрыть"));
}

UserGuide::~UserGuide()
{
    delete ui;
}

void UserGuide::buildContents()
{
    ui->treeContents->clear();
    ui->treeContents->setHeaderHidden(true);

    auto addItem = [&](const QString& title, const QString& key, QTreeWidgetItem* parent = nullptr)
    {
        QTreeWidgetItem* it = parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem(ui->treeContents);
        it->setText(0, title);
        it->setData(0, Qt::UserRole, key);
        return it;
    };

    addItem(tr("Введение"), "intro");
    addItem(tr("Интерфейс"), "ui");
    addItem(tr("Создание разметки"), "create");
    addItem(tr("Редактирование"), "edit");
    addItem(tr("Горячие клавиши"), "shortcuts");
    addItem(tr("Настройки"), "settings");
    addItem(tr("FAQ"), "faq");

    ui->treeContents->expandAll();
    ui->treeContents->setCurrentItem(ui->treeContents->topLevelItem(0));
}

void UserGuide::showPage(const QString& key)
{
    if (key == "intro")
    {
        ui->textBrowser->setHtml(
            "<h2>Введение</h2>"
            "<p><b>QLabelMe</b> — инструмент для разметки изображений: создание и редактирование фигур, "
            "привязка к классам и сохранение аннотаций.</p>"
        );
        return;
    }

    if (key == "shortcuts")
    {
        ui->textBrowser->setHtml(
            "<h2>Горячие клавиши</h2>"
            "<h3>Редактирование</h3>"
            "<table border='1' cellspacing='0' cellpadding='6'>"
            "<tr><th>Клавиша</th><th>Действие</th></tr>"
            "<tr><td><code>Ctrl + Z</code></td><td>Отменить</td></tr>"
            "<tr><td><code>Ctrl + Shift + Z</code></td><td>Повторить</td></tr>"
            "<tr><td><code>Ctrl + 0</code></td><td>Сброс масштаба</td></tr>"
            "</table>"
        );
        return;
    }

    // Заглушки для остальных разделов
    ui->textBrowser->setHtml(QString("<h2>%1</h2><p>Раздел в разработке.</p></h2>").arg(key));
}
