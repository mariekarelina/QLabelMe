#include "unsaved_changes.h"
#include "ui_unsaved_changes.h"

#include "load_geometry.h"
#include "shared/config/appl_conf.h"

#include <QAbstractButton>
#include <QGuiApplication>
#include <QSignalBlocker>
#include <QPushButton>
#include <QCheckBox>
#include <QFileInfo>
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

    for (const QString& path : documents)
    {
        QListWidgetItem* item = new QListWidgetItem(QFileInfo(path).fileName());

        // Храним полный путь отдельно от отображаемого имени
        item->setData(Qt::UserRole, path);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Checked);
        ui->unsavedDocs->addItem(item);
    }

    // Все файлы отмечены при открытии окна
    ui->selectAllCheckBox->setCheckState(Qt::Checked);

    connect(ui->selectAllCheckBox, &QCheckBox::clicked, this, [this](bool checked)
    {
        // При массовом выборе не пересчитываем состояние после каждой строки
        const QSignalBlocker blocker(ui->unsavedDocs);
        const Qt::CheckState state = checked ? Qt::Checked : Qt::Unchecked;

        for (int row = 0; row < ui->unsavedDocs->count(); ++row)
            ui->unsavedDocs->item(row)->setCheckState(state);

        ui->selectAllCheckBox->setTristate(false);
    });

    connect(ui->unsavedDocs, &QListWidget::itemChanged, this, [this]()
    {
        int checkedCount = 0;
        const int itemCount = ui->unsavedDocs->count();

        for (int row = 0; row < itemCount; ++row)
        {
            if (ui->unsavedDocs->item(row)->checkState() == Qt::Checked)
                ++checkedCount;
        }

        // Промежуточное состояние означает, что отмечена часть файлов
        const bool partiallyChecked = checkedCount > 0 && checkedCount < itemCount;
        ui->selectAllCheckBox->setTristate(partiallyChecked);

        if (partiallyChecked)
            ui->selectAllCheckBox->setCheckState(Qt::PartiallyChecked);
        else
            ui->selectAllCheckBox->setCheckState(checkedCount > 0 ? Qt::Checked : Qt::Unchecked);
    });

    QPushButton* saveAllButton = ui->buttonBox->button(QDialogButtonBox::SaveAll);
    if (saveAllButton)
        saveAllButton->setText(u8"Сохранить выбранные");

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
    //std::vector<int> wsc23 = windowScreenCenter23();
    dialogLoadGeometry(config::base(), "windows.unsaved_changes.geometry", this);

    // QVector<int> wg;
    // config::base().getValue("windows.unsaved_changes.geometry", wg);

    // if (wg.count() != 4)
    // {
    //     wg = {10, 10, 600, 400};

    //     const QList<QScreen*> screens = QGuiApplication::screens();
    //     if (!screens.isEmpty())
    //     {
    //         const QRect screenGeometry = screens[0]->availableGeometry();

    //         QRect windowGeometry {
    //             0,
    //             0,
    //             int(screenGeometry.width() * 0.40),
    //             int(screenGeometry.height() * 0.35)
    //         };

    //         windowGeometry.setWidth(qMax(windowGeometry.width(), 600));
    //         windowGeometry.setHeight(qMax(windowGeometry.height(), 400));

    //         wg[0] = screenGeometry.x() + screenGeometry.width() / 2 - windowGeometry.width() / 2;
    //         wg[1] = screenGeometry.y() + screenGeometry.height() / 2 - windowGeometry.height() / 2;
    //         wg[2] = windowGeometry.width();
    //         wg[3] = windowGeometry.height();
    //     }
    // }

    // setGeometry(wg[0], wg[1], wg[2], wg[3]);
}

void UnsavedChanges::saveGeometry() const
{
    // const QRect r = (isMaximized() || isFullScreen())
    //                 ? normalGeometry()
    //                 : geometry();

    // QVector<int> out {r.x(), r.y(), r.width(), r.height()};
    // config::base().setValue("windows.unsaved_changes.geometry", out);
    // config::base().saveFile();

    QRect g = geometry();
    QVector<int> wg {g.x(), g.y(), g.width(), g.height()};
    config::base().setValue("windows.unsaved_changes.geometry", wg);
}

QSet<QString> UnsavedChanges::checkedDocumentPaths() const
{
    QSet<QString> paths;

    for (int row = 0; row < ui->unsavedDocs->count(); ++row)
    {
        const QListWidgetItem* item = ui->unsavedDocs->item(row);

        if (item->checkState() == Qt::Checked)
            paths.insert(item->data(Qt::UserRole).toString());
    }

    return paths;
}

QDialogButtonBox::StandardButton UnsavedChanges::selectedButton() const
{
    return _selectedButton;
}
