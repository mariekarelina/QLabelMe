#include "unsaved_changes.h"

#include "ui_unsaved_changes.h"
#include "shared/config/appl_conf.h"

#include <QAbstractButton>
#include <QPushButton>
#include <QGuiApplication>
#include <QPushButton>
#include <QScreen>

UnsavedChanges::UnsavedChanges(const QStringList& documents, QWidget* parent)
    : QDialog(parent),
      ui(new Ui::UnsavedChanges)
{
    ui->setupUi(this);

    setWindowTitle(u8"Несохраненные изменения");
    setModal(true);
    loadGeometry();

    ui->unsavedDocs->clear();

    for (const QString& document : documents)
        ui->unsavedDocs->addItem(document);

    QPushButton* saveAllButton = ui->buttonBox->button(QDialogButtonBox::SaveAll);
    if (saveAllButton)
        saveAllButton->setText(u8"Сохранить все");

    QPushButton* discardButton = ui->buttonBox->button(QDialogButtonBox::Discard);
    if (discardButton)
        discardButton->setText(u8"Не сохранять");

    QPushButton* cancelButton = ui->buttonBox->button(QDialogButtonBox::Cancel);
    if (cancelButton)
        cancelButton->setText(u8"Отмена");

    connect(ui->buttonBox, &QDialogButtonBox::clicked,
            this, [this](QAbstractButton* button)
    {
        _selectedButton = ui->buttonBox->standardButton(button);

        if (_selectedButton == QDialogButtonBox::Cancel)
        {
            reject();
            return;
        }

        accept();
    });
}

UnsavedChanges::~UnsavedChanges()
{
    delete ui;
}

void UnsavedChanges::closeEvent(QCloseEvent* event)
{
    saveGeometry();
    QDialog::closeEvent(event);
}

void UnsavedChanges::accept()
{
    saveGeometry();
    QDialog::accept();
}

void UnsavedChanges::reject()
{
    saveGeometry();
    QDialog::reject();
}

void UnsavedChanges::loadGeometry()
{
    QVector<int> wg;
    config::base().getValue("windows.unsaved_changes.geometry", wg);

    if (wg.count() != 4)
    {
        wg = {10, 10, 600, 400};

        const QList<QScreen*> screens = QGuiApplication::screens();
        if (!screens.isEmpty())
        {
            const QRect screenGeometry = screens[0]->availableGeometry();

            QRect windowGeometry {
                0,
                0,
                int(screenGeometry.width() * 0.40),
                int(screenGeometry.height() * 0.35)
            };

            windowGeometry.setWidth(qMax(windowGeometry.width(), 600));
            windowGeometry.setHeight(qMax(windowGeometry.height(), 400));

            wg[0] = screenGeometry.x() + screenGeometry.width() / 2 - windowGeometry.width() / 2;
            wg[1] = screenGeometry.y() + screenGeometry.height() / 2 - windowGeometry.height() / 2;
            wg[2] = windowGeometry.width();
            wg[3] = windowGeometry.height();
        }
    }

    setGeometry(wg[0], wg[1], wg[2], wg[3]);
}

void UnsavedChanges::saveGeometry() const
{
    const QRect r = (isMaximized() || isFullScreen())
                    ? normalGeometry()
                    : geometry();

    QVector<int> out { r.x(), r.y(), r.width(), r.height() };
    config::base().setValue("windows.unsaved_changes.geometry", out);
    config::base().saveFile();
}

QDialogButtonBox::StandardButton UnsavedChanges::selectedButton() const
{
    return _selectedButton;
}
