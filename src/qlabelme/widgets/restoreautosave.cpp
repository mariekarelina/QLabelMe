#include "restoreautosave.h"
#include "ui_restoreautosave.h"

#include "load_geometry.h"
#include "shared/config/appl_conf.h"

#include <QDialogButtonBox>
#include <QPushButton>
#include <QListWidget>
#include <QFileInfo>
#include <QRect>
#include <QVector>
#include <QCheckBox>
#include <QSignalBlocker>

RestoreAutoSave::RestoreAutoSave(const QStringList& documents,
                                 QWidget* parent)
    : QDialog(parent),
      ui(new Ui::RestoreAutoSave)
{
    ui->setupUi(this);

    setWindowTitle(u8"Восстановление разметки");

    for (const QString& path : documents)
    {
        QListWidgetItem* item = new QListWidgetItem(QFileInfo(path).fileName());

        // Храним полный путь отдельно от имени, показанного в списке
        item->setData(Qt::UserRole, path);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Checked);
        ui->autosaveDocs->addItem(item);
    }

    ui->selectAllCheckBox->setCheckState(Qt::Checked);
    connect(ui->selectAllCheckBox, &QCheckBox::clicked, this, [this](bool checked)
    {
        // При массовом выборе не пересчитываем общий чекбокс для каждой строки
        const QSignalBlocker blocker(ui->autosaveDocs);
        const Qt::CheckState state = checked ? Qt::Checked : Qt::Unchecked;

        for (int row = 0; row < ui->autosaveDocs->count(); ++row)
            ui->autosaveDocs->item(row)->setCheckState(state);

        ui->selectAllCheckBox->setTristate(false);
    });

    connect(ui->autosaveDocs, &QListWidget::itemChanged, this, [this]()
    {
        int checkedCount = 0;
        const int itemCount = ui->autosaveDocs->count();

        for (int row = 0; row < itemCount; ++row)
        {
            if (ui->autosaveDocs->item(row)->checkState() == Qt::Checked)
                ++checkedCount;
        }

        // Промежуточное состояние показываем только при частичном выборе
        const bool partiallyChecked = checkedCount > 0 && checkedCount < itemCount;
        ui->selectAllCheckBox->setTristate(partiallyChecked);

        if (partiallyChecked)
            ui->selectAllCheckBox->setCheckState(Qt::PartiallyChecked);
        else
            ui->selectAllCheckBox->setChecked(checkedCount > 0);
    });

    QPushButton* restoreButton =
        ui->buttonBox->button(QDialogButtonBox::Yes);

    if (restoreButton)
    {
        restoreButton->setText(QStringLiteral("Да"));
        restoreButton->setDefault(true);
    }

    QPushButton* openSavedButton =
        ui->buttonBox->button(QDialogButtonBox::No);

    if (openSavedButton)
        openSavedButton->setText(QStringLiteral("Нет"));

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

QSet<QString> RestoreAutoSave::checkedAutoSavePaths() const
{
    QSet<QString> paths;

    // Собираем пути только для отмеченных строк
    for (int row = 0; row < ui->autosaveDocs->count(); ++row)
    {
        const QListWidgetItem* item = ui->autosaveDocs->item(row);

        if (item->checkState() == Qt::Checked)
            paths.insert(item->data(Qt::UserRole).toString());
    }

    return paths;
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
