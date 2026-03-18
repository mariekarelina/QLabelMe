#include "user_guide.h"
#include "ui_user_guide.h"
#include "shared/config/appl_conf.h"
#include <QTreeWidgetItem>
#include <QPushButton>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QMap>
#include <QCloseEvent>
#include <QGuiApplication>
#include <QScreen>
#include <QSplitter>


UserGuide::UserGuide(QWidget *parent)
    : QDialog(parent),
      ui(new Ui::UserGuide)
{
    ui->setupUi(this);

    setWindowTitle(tr("Руководство пользователя QLabelMe"));
    setAttribute(Qt::WA_DeleteOnClose);

    loadGeometry();
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
        this, &UserGuide::reject);
    ui->buttonBox->button(QDialogButtonBox::Close)->setText(tr("Закрыть"));
}

UserGuide::~UserGuide()
{
    delete ui;
}

void UserGuide::closeEvent(QCloseEvent* event)
{
    saveGeometry();
    QDialog::closeEvent(event);
}

void UserGuide::reject()
{
    saveGeometry();
    QDialog::reject();
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

void UserGuide::loadGeometry()
{
    QVector<int> wg;
    config::base().getValue("windows.user_guide.geometry", wg);

    if (wg.count() != 4)
    {
        wg = {10, 10, 900, 600};

        const QList<QScreen*> screens = QGuiApplication::screens();
        if (!screens.isEmpty())
        {
            const QRect screenGeometry = screens[0]->geometry();
            QRect windowGeometry {0, 0,
                                  int(screenGeometry.width() * (2.0 / 3.0)),
                                  int(screenGeometry.height() * (2.0 / 3.0))};

            wg[0] = screenGeometry.width()  / 2 - windowGeometry.width()  / 2;
            wg[1] = screenGeometry.height() / 2 - windowGeometry.height() / 2;
            wg[2] = windowGeometry.width();
            wg[3] = windowGeometry.height();
        }
    }

    setGeometry(wg[0], wg[1], wg[2], wg[3]);

    QList<int> splitterSizes;
    if (!config::base().getValue("windows.user_guide.splitter_sizes", splitterSizes))
    {
        splitterSizes = {100, 100};
        const QRect windowGeometry = geometry();
        splitterSizes[0] = int(windowGeometry.width() * (1.0 / 3.0));
        splitterSizes[1] = int(windowGeometry.width() * (2.0 / 3.0));
    }

    ui->splitter->setSizes(splitterSizes);
}

void UserGuide::saveGeometry() const
{
    const QRect r = (isMaximized() || isFullScreen())
                    ? normalGeometry()
                    : geometry();

    QVector<int> out { r.x(), r.y(), r.width(), r.height() };
    config::base().setValue("windows.user_guide.geometry", out);

    config::base().setValue("windows.user_guide.splitter_sizes",
                            ui->splitter->sizes());

    config::base().saveFile();
}
