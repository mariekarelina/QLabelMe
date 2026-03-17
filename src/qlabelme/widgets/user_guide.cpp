#include "user_guide.h"
#include "ui_user_guide.h"
#include <QTreeWidgetItem>
#include <QPushButton>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QMap>


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

    ui->treeContents->expandAll();
    ui->treeContents->setCurrentItem(ui->treeContents->topLevelItem(0));
}

static QString extractSectionByTitle(const QString& markdown, const QString& title)
{
    const QStringList lines = markdown.split('\n');

    QStringList result;
    bool inSection = false;

    for (const QString& line : lines)
    {
        const QString trimmed = line.trimmed();

        // Начало нужного раздела
        if (!inSection)
        {
            if (trimmed == "# " + title || trimmed == "## " + title)
            {
                inSection = true;
                result << line;
            }
            continue;
        }

        // Конец раздела: встретили следующий заголовок того же или более высокого уровня
        if (trimmed.startsWith("# ") || trimmed.startsWith("## "))
        {
            break;
        }

        result << line;
    }

    return result.join('\n').trimmed();
}

void UserGuide::showPage(const QString& key)
{
    const QString sourceDir =
        QFileInfo(QString::fromUtf8(__FILE__)).absolutePath();
    const QString mdPath = sourceDir + "/user_guide.md";

    QFile file(mdPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        ui->textBrowser->setHtml(
            QString("<h2>Ошибка</h2><p>Не удалось открыть файл документации:<br><code>%1</code></p>")
                .arg(mdPath.toHtmlEscaped())
        );
        return;
    }

    QTextStream in(&file);
    in.setCodec("UTF-8");
    const QString markdown = in.readAll();

    const QMap<QString, QString> sectionTitles = {
        {"intro", "Введение"},
        {"ui", "Интерфейс программы"},
        {"create", "Создание разметки"},
        {"edit", "Редактирование разметки"},
        {"shortcuts", "Горячие клавиши"},
        {"settings", "Настройки"}
    };

    const QString title = sectionTitles.value(key);
    if (title.isEmpty())
    {
        ui->textBrowser->setMarkdown(markdown);
        return;
    }

    const QString sectionMarkdown = extractSectionByTitle(markdown, title);

    if (sectionMarkdown.isEmpty())
    {
        ui->textBrowser->setHtml(
            QString("<h2>Ошибка</h2><p>Раздел <b>%1</b> не найден в файле документации.</p>")
                .arg(title.toHtmlEscaped())
        );
        return;
    }

    ui->textBrowser->setMarkdown(sectionMarkdown);
}
