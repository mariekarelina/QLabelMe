#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include <QDialogButtonBox>
#include <QPushButton>
#include <QColorDialog>
#include <QVBoxLayout>
#include <QtWidgets>
#include <QAbstractButton>
#include <QButtonGroup>
#include <QRegularExpression>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent),
      ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("ОК"));
    ui->buttonBox->button(QDialogButtonBox::Apply)->setText(tr("Применить"));
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Отмена"));
    wireUi();
    applyModelToUi();
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::wireUi()
{
    const auto all = this->findChildren<QPushButton*>(
        QRegularExpression("^pushButton(_\\d+)?$")
    );
    auto byName = all;
    std::sort(byName.begin(), byName.end(),
              [](QPushButton* a, QPushButton* b){ return a->objectName() < b->objectName(); });

    if (byName.size() >= 7)
    {
        m_btnNode     = byName.at(0);
        m_btnNodeSel  = byName.at(1);
        m_btnNumber   = byName.at(2);
        m_btnNumberBg = byName.at(3);
        m_btnRect     = byName.at(4);
        m_btnCircle   = byName.at(5);
        m_btnPolyline = byName.at(6);
    }

    // Привязываем диалог выбора цвета
    attachColorPicker(m_btnNode, &m_values.nodeColor);
    attachColorPicker(m_btnNodeSel, &m_values.nodeSelectedColor);
    attachColorPicker(m_btnNumber, &m_values.numberColor);
    attachColorPicker(m_btnNumberBg, &m_values.numberBgColor);
    attachColorPicker(m_btnRect, &m_values.rectLineColor);
    attachColorPicker(m_btnCircle, &m_values.circleLineColor);
    attachColorPicker(m_btnPolyline, &m_values.polylineLineColor);

    // Кнопки диалога
    connect(ui->buttonBox, &QDialogButtonBox::accepted,
            this, &SettingsDialog::on_buttonBox_accepted);
    connect(ui->buttonBox, &QDialogButtonBox::rejected,
            this, &SettingsDialog::on_buttonBox_rejected);
    connect(ui->buttonBox, &QDialogButtonBox::clicked,
            this, &SettingsDialog::on_buttonBox_clicked);

    // Радиокнопки «Замыкание полилинии»
    setupCloseMethodGroup();
}

void SettingsDialog::setupCloseMethodGroup()
{
    auto *g = new QButtonGroup(this);
    g->setExclusive(true);

    g->addButton(ui->doubleClickLMB, static_cast<int>(PolylineCloseMode::DoubleClick));
    g->addButton(ui->LMBZeroPoint, static_cast<int>(PolylineCloseMode::SingleClickOnFirstPoint));
    g->addButton(ui->holdingCtrl, static_cast<int>(PolylineCloseMode::CtrlModifier));
    g->addButton(ui->keyC, static_cast<int>(PolylineCloseMode::KeyCWithoutNewPoint));

    connect(g, &QButtonGroup::idClicked,
            this, [this](int id){
                m_values.closeMethod = static_cast<PolylineCloseMode>(id);
            });
}

void SettingsDialog::setValues(const Values &v)
{
    m_values = v;
    applyModelToUi();
}

void SettingsDialog::applyModelToUi()
{
    ui->spinLineWidth->setValue(m_values.lineWidth);
    ui->spinHandleSize->setValue(m_values.handleSize);
    ui->spinNumberSize->setValue(m_values.numberFontPt);

    if (m_btnNode)
        updateColorPreview(m_btnNode, m_values.nodeColor);
    if (m_btnNodeSel)
        updateColorPreview(m_btnNodeSel, m_values.nodeSelectedColor);
    if (m_btnNumber)
        updateColorPreview(m_btnNumber, m_values.numberColor);
    if (m_btnNumberBg)
        updateColorPreview(m_btnNumberBg, m_values.numberBgColor);
    if (m_btnRect)
        updateColorPreview(m_btnRect, m_values.rectLineColor);
    if (m_btnCircle)
        updateColorPreview(m_btnCircle, m_values.circleLineColor);
    if (m_btnPolyline)
        updateColorPreview(m_btnPolyline, m_values.polylineLineColor);

    switch (m_values.closeMethod)
    {
        case PolylineCloseMode::DoubleClick:
            ui->doubleClickLMB->setChecked(true);
            break;
        case PolylineCloseMode::SingleClickOnFirstPoint:
            ui->LMBZeroPoint->setChecked(true);
            break;
        case PolylineCloseMode::CtrlModifier:
            ui->holdingCtrl->setChecked(true);
            break;
        case PolylineCloseMode::KeyCWithoutNewPoint:
            ui->keyC->setChecked(true);
            break;
    }
}

void SettingsDialog::applyUiToModel()
{
    m_values.lineWidth = ui->spinLineWidth->value();
    m_values.handleSize = ui->spinHandleSize->value();
    m_values.numberFontPt = ui->spinNumberSize->value();

    if (ui->doubleClickLMB->isChecked())
        m_values.closeMethod = PolylineCloseMode::DoubleClick;
    else if (ui->LMBZeroPoint->isChecked())
        m_values.closeMethod = PolylineCloseMode::SingleClickOnFirstPoint;
    else if (ui->holdingCtrl->isChecked())
        m_values.closeMethod = PolylineCloseMode::CtrlModifier;
    else if (ui->keyC->isChecked())
        m_values.closeMethod = PolylineCloseMode::KeyCWithoutNewPoint;
}

void SettingsDialog::on_buttonBox_accepted()
{
    applyUiToModel();
    emit settingsApplied(m_values);
    accept();
}

void SettingsDialog::on_buttonBox_rejected()
{
    reject();
}

void SettingsDialog::on_buttonBox_clicked(QAbstractButton *btn)
{
    const auto role = ui->buttonBox->buttonRole(btn);
    if (role == QDialogButtonBox::ApplyRole)
    {
        applyUiToModel();
        emit settingsApplied(m_values);
    }
}

void SettingsDialog::updateColorPreview(QPushButton *btn, const QColor &c)
{
    if (!btn) return;
    btn->setStyleSheet(
        QStringLiteral("border:1px solid #777;"
                       "background:%1;"
                       "min-width:26px; max-width:26px;"
                       "min-height:22px; max-height:22px;")
        .arg(c.name(QColor::HexRgb))
    );
}

bool SettingsDialog::pickColor(QWidget *parent, const QString &title,
                               QColor &ioColor, QPushButton *previewBtn)
{
    const QColor chosen = QColorDialog::getColor(ioColor, parent, title,
                                                 QColorDialog::ShowAlphaChannel);
    if (!chosen.isValid())
        return false;
    ioColor = chosen;
    updateColorPreview(previewBtn, ioColor);
    return true;
}

void SettingsDialog::attachColorPicker(QPushButton *btn, QColor *target)
{
    if (!btn || !target) return;
    updateColorPreview(btn, *target);
    connect(btn, &QPushButton::clicked, this, [this, btn, target]{
        pickColor(this, tr("Выбор цвета"), *target, btn);
    });
}
