#include "restoreautosave.h"
#include "ui_restoreautosave.h"

#include "load_geometry.h"
#include "shared/config/appl_conf.h"

#include <QDialogButtonBox>
#include <QPushButton>
#include <QRect>
#include <QVector>

RestoreAutoSave::RestoreAutoSave(const QStringList& documents,
                                 QWidget* parent)
    : QDialog(parent),
      ui(new Ui::RestoreAutoSave)
{
    ui->setupUi(this);

    setWindowTitle(u8"Восстановление разметки");

    ui->messageLabel->setText(
        QString::fromUtf8(
            u8"Найдены резервные копии разметки: %1.\n"
            u8"Восстановить их?\n\n"
            u8"Основные файлы будут заменены только "
            u8"после обычного сохранения."
        ).arg(documents.size())
    );

    ui->autosaveDocs->addItems(documents);

    QPushButton* restoreButton =
        ui->buttonBox->button(QDialogButtonBox::Ok);

    if (restoreButton)
    {
        restoreButton->setText(u8"Восстановить");
        restoreButton->setDefault(true);
    }

    QPushButton* openSavedButton =
        ui->buttonBox->button(QDialogButtonBox::Cancel);

    if (openSavedButton)
        openSavedButton->setText(u8"Открыть сохранённые");

    connect(ui->buttonBox, &QDialogButtonBox::accepted,
            this, &RestoreAutoSave::accept);

    connect(ui->buttonBox, &QDialogButtonBox::rejected,
            this, [this]()
    {
        // Удаление разрешено только при нажатии кнопки
        _discardAutoSaves = true;
        reject();
    });

    // Восстанавливаем размер и положение окна
    dialogLoadGeometry(
        config::base(),
        "windows.restore_auto_save.geometry",
        this
    );
}

RestoreAutoSave::~RestoreAutoSave()
{
    delete ui;
}

bool RestoreAutoSave::discardAutoSaves() const
{
    return _discardAutoSaves;
}

void RestoreAutoSave::done(int result)
{
    // Сохраняем геометрию при любом способе закрытия диалога
    const QRect rect = isMaximized() || isFullScreen()
        ? normalGeometry()
        : geometry();

    const QVector<int> values {
        rect.x(),
        rect.y(),
        rect.width(),
        rect.height()
    };

    config::base().setValue(
        "windows.restore_auto_save.geometry",
        values
    );

    QDialog::done(result);
}
