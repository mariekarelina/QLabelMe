#include "project_settings.h"
#include "ui_project_settings.h"

ProjectSettings::ProjectSettings(QWidget* parent)
    : QDialog(parent),
      ui(new Ui::ProjectSettings)
{
    ui->setupUi(this);

    // Всегда открываемся на первой вкладке
    if (ui->tabWidget)
        ui->tabWidget->setCurrentIndex(0);
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
        ui->listClasses->clear();
        ui->listClasses->addItems(m_classes);
    }
}
