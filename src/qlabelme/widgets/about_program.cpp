#include "about_program.h"
#include "ui_about_program.h"
#include "license.h"

#include "shared/config/appl_conf.h"

#include <QCloseEvent>
#include <QDialogButtonBox>
#include <QGuiApplication>
#include <QIcon>
#include <QPushButton>
#include <QScreen>

AboutProgram::AboutProgram(QWidget* parent)
    : QDialog(parent),
      ui(new Ui::AboutProgram)
{
    ui->setupUi(this);

    // При открытии окна всегда показываем первую вкладку
    ui->tabWidget->setCurrentIndex(0);

    setWindowTitle(u8"О программе QLabelMe");
    setWindowIcon(QIcon(":/images/resources/qlabelme.png"));
    setAttribute(Qt::WA_DeleteOnClose);

    loadGeometry();

    ui->titleLabel->setText(u8"<b>QLabelMe</b>");

    ui->versionLabel->setText(
        QString(u8"Версия: %1<br>"
                u8"Git revision: %2<br>"
                u8"Qt: %3<br>"
                u8"Лицензия: <a href=\"license\">GNU GPL v3</a>")
            .arg(VERSION_PROJECT)
            .arg(GIT_REVISION)
            .arg(QT_VERSION_STR));

    // Обрабатываем ссылку внутри приложения
    ui->versionLabel->setOpenExternalLinks(false);
    ui->versionLabel->setTextInteractionFlags(Qt::LinksAccessibleByMouse);

    connect(ui->versionLabel, &QLabel::linkActivated,
            this, [this](const QString& link)
    {
        if (link != "license")
            return;

        License dialog(this);
        dialog.exec();
    });

    ui->descriptionBrowser->setOpenExternalLinks(true);
    ui->descriptionBrowser->setHtml(
        u8"<p>QLabelMe - программа для ручной разметки и аннотирования изображений.</p>"
        u8"<p>Программа позволяет создавать и редактировать графические примитивы, "
        u8"назначать им классы, сохранять результат в YAML-файлы и восстанавливать "
        u8"разметку при повторном открытии проекта.</p>"
        u8"<p>QLabelMe используется для подготовки наборов данных в задачах "
        u8"компьютерного зрения и машинного обучения.</p>"
        u8"<p><b>Обратная связь:</b><br>"
        u8"<a href=\"https://github.com/mariekarelina/QLabelMe/issues\">"
        u8"https://github.com/mariekarelina/QLabelMe/issues</a></p>");

    ui->authorsBrowser->setHtml(
        u8"<ul>"
        u8"<li>Карелина Мария - автор, разработчик</li>"
        u8"<li>Карелин Павел - консультант по коду и архитектуре программы</li>"
        u8"<li>Назаровский Александр - консультант по функциональной части программы</li>"
        u8"</ul>");

    connect(ui->buttonBox, &QDialogButtonBox::rejected,
        this, &AboutProgram::reject);

    ui->buttonBox->button(QDialogButtonBox::Close)->setText(u8"Закрыть");
}

AboutProgram::~AboutProgram()
{
    delete ui;
}

void AboutProgram::closeEvent(QCloseEvent* event)
{
    saveGeometry();
    QDialog::closeEvent(event);
}

void AboutProgram::accept()
{
    saveGeometry();
    QDialog::accept();
}

void AboutProgram::reject()
{
    saveGeometry();
    QDialog::reject();
}

void AboutProgram::loadGeometry()
{
    QVector<int> wg;
    config::base().getValue("windows.about_program.geometry", wg);

    if (wg.count() != 4)
    {
        wg = {10, 10, 600, 500};

        const QList<QScreen*> screens = QGuiApplication::screens();
        if (!screens.isEmpty())
        {
            const QRect screenGeometry = screens[0]->availableGeometry();

            QRect windowGeometry {
                0,
                0,
                int(screenGeometry.width() * 0.45),
                int(screenGeometry.height() * 0.55)
            };

            windowGeometry.setWidth(qMax(windowGeometry.width(), 600));
            windowGeometry.setHeight(qMax(windowGeometry.height(), 500));

            wg[0] = screenGeometry.x() + screenGeometry.width() / 2 - windowGeometry.width() / 2;
            wg[1] = screenGeometry.y() + screenGeometry.height() / 2 - windowGeometry.height() / 2;
            wg[2] = windowGeometry.width();
            wg[3] = windowGeometry.height();
        }
    }

    setGeometry(wg[0], wg[1], wg[2], wg[3]);
}

void AboutProgram::saveGeometry() const
{
    const QRect r = (isMaximized() || isFullScreen())
                    ? normalGeometry()
                    : geometry();

    QVector<int> out { r.x(), r.y(), r.width(), r.height() };
    config::base().setValue("windows.about_program.geometry", out);
    config::base().saveFile();
}
