#include "settings.h"
#include "ui_settings.h"
#include <QDialogButtonBox>
#include <QPushButton>
#include <QColorDialog>
#include <QVBoxLayout>
#include <QtWidgets>
#include <QAbstractButton>
#include <QButtonGroup>
#include <QRegularExpression>
#include <QShowEvent>

Settings::Settings(QWidget *parent)
    : QDialog(parent),
      ui(new Ui::Settings)
{
    ui->setupUi(this);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("ОК"));
    ui->buttonBox->button(QDialogButtonBox::Apply)->setText(tr("Применить"));
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Отмена"));

    wireUi();
    applyModelToUi();
}

Settings::~Settings()
{
    delete ui;
}

void Settings::showEvent(QShowEvent* e)
{
    QDialog::showEvent(e);
    // Всегда открываемся на первой вкладке
    if (ui && ui->tabWidget)
        ui->tabWidget->setCurrentIndex(0);   // 0 — «Общее»
}

void Settings::wireUi()
{
    const auto all = this->findChildren<QPushButton*>(
        QRegularExpression("^pushButton(_\\d+)?$")
    );
    auto byName = all;
    std::sort(byName.begin(), byName.end(),
              [](QPushButton* a, QPushButton* b){ return a->objectName() < b->objectName(); });

    if (byName.size() >= 9)
    {
        _btnNode = byName.at(0);
        _btnNodeSel = byName.at(1);
        _btnNumber = byName.at(2);
        _btnNumberBg = byName.at(3);
        _btnRect = byName.at(4);
        _btnCircle = byName.at(5);
        _btnPolyline = byName.at(6);
        _btnLine = byName.at(7);
        _btnPoint = byName.at(8);
    }

    // Привязываем диалог выбора цвета
    attachColorPicker(_btnNode, &_values.nodeColor);
    attachColorPicker(_btnNodeSel, &_values.nodeSelectedColor);
    attachColorPicker(_btnNumber, &_values.numberColor);
    attachColorPicker(_btnNumberBg, &_values.numberBgColor);
    attachColorPicker(_btnRect, &_values.rectLineColor);
    attachColorPicker(_btnCircle, &_values.circleLineColor);
    attachColorPicker(_btnPolyline, &_values.polylineLineColor);
    attachColorPicker(_btnLine,  &_values.lineLineColor);
    attachColorPicker(_btnPoint, &_values.pointColor);

    // Кнопки диалога
    connect(ui->buttonBox, &QDialogButtonBox::clicked,
            this, &Settings::on_buttonBox_clicked);
    connect(ui->buttonBox, &QDialogButtonBox::accepted,
            this, &Settings::on_buttonBox_accepted);
    connect(ui->buttonBox, &QDialogButtonBox::rejected,
            this, &Settings::on_buttonBox_rejected);


    // Радиокнопки «Замыкание полилинии»
    setupCloseMethodGroup();
    // Радиокнопки "Завршение линии"
    setupFinishMethodGroup();
}

void Settings::setupCloseMethodGroup()
{
    auto *g = new QButtonGroup(this);
    g->setExclusive(true);

    g->addButton(ui->doubleClickLMB, static_cast<int>(PolylineCloseMode::DoubleClick));
    g->addButton(ui->LMBZeroPoint, static_cast<int>(PolylineCloseMode::SingleClickOnFirstPoint));
    g->addButton(ui->holdingCtrl, static_cast<int>(PolylineCloseMode::CtrlModifier));
    g->addButton(ui->keyC, static_cast<int>(PolylineCloseMode::KeyCWithoutNewPoint));

    connect(g, &QButtonGroup::idClicked,
            this, [this](int id){
                _values.closePolyline = static_cast<PolylineCloseMode>(id);
            });
}

void Settings::setupFinishMethodGroup()
{
    auto *g = new QButtonGroup(this);
    g->setExclusive(true);

    g->addButton(ui->doubleClickLMB_2, static_cast<int>(LineFinishMode::DoubleClick));
    g->addButton(ui->holdingCtrl_2, static_cast<int>(LineFinishMode::CtrlModifier));
    g->addButton(ui->keyC_2, static_cast<int>(LineFinishMode::KeyCWithoutNewPoint));

    connect(g, &QButtonGroup::idClicked,
            this, [this](int id){
                _values.finishLine = static_cast<LineFinishMode>(id);
            });
}

void Settings::setValues(const Values &v)
{
    _values = v;
    applyModelToUi();
}

void Settings::applyModelToUi()
{
    ui->spinLineWidth->setValue(_values.lineWidth);
    ui->spinHandleSize->setValue(_values.handleSize);
    ui->spinNumberSize->setValue(_values.numberFontPt);
    ui->spinPointSize->setValue(_values.pointSize);

    if (_btnNode)
        updateColorPreview(_btnNode, _values.nodeColor);
    if (_btnNodeSel)
        updateColorPreview(_btnNodeSel, _values.nodeSelectedColor);
    if (_btnNumber)
        updateColorPreview(_btnNumber, _values.numberColor);
    if (_btnNumberBg)
        updateColorPreview(_btnNumberBg, _values.numberBgColor);
    if (_btnRect)
        updateColorPreview(_btnRect, _values.rectLineColor);
    if (_btnCircle)
        updateColorPreview(_btnCircle, _values.circleLineColor);
    if (_btnPolyline)
        updateColorPreview(_btnPolyline, _values.polylineLineColor);
    if (_btnLine)
        updateColorPreview(_btnLine, _values.lineLineColor);
    if (_btnPoint)
        updateColorPreview(_btnPoint, _values.pointColor);

    switch (_values.closePolyline)
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
    switch (_values.finishLine)
    {
        case LineFinishMode::DoubleClick:
            ui->doubleClickLMB_2->setChecked(true);
            break;
        case LineFinishMode::CtrlModifier:
            ui->holdingCtrl_2->setChecked(true);
            break;
        case LineFinishMode::KeyCWithoutNewPoint:
            ui->keyC_2->setChecked(true);
            break;
    }
}

void Settings::applyUiToModel()
{
    _values.lineWidth = ui->spinLineWidth->value();
    _values.handleSize = ui->spinHandleSize->value();
    _values.numberFontPt = ui->spinNumberSize->value();
    _values.pointSize = ui->spinPointSize->value();

    if (ui->doubleClickLMB->isChecked())
        _values.closePolyline = PolylineCloseMode::DoubleClick;
    else if (ui->LMBZeroPoint->isChecked())
        _values.closePolyline = PolylineCloseMode::SingleClickOnFirstPoint;
    else if (ui->holdingCtrl->isChecked())
        _values.closePolyline = PolylineCloseMode::CtrlModifier;
    else if (ui->keyC->isChecked())
        _values.closePolyline = PolylineCloseMode::KeyCWithoutNewPoint;

    if (ui->doubleClickLMB_2->isChecked())
        _values.finishLine = LineFinishMode::DoubleClick;
    else if (ui->holdingCtrl_2->isChecked())
        _values.finishLine = LineFinishMode::CtrlModifier;
    else if (ui->keyC_2->isChecked())
        _values.finishLine = LineFinishMode::KeyCWithoutNewPoint;
}

void Settings::on_buttonBox_accepted()
{
    applyUiToModel();
    emit settingsApplied(_values);
    accept();
}

void Settings::on_buttonBox_rejected()
{
    reject();
}

void Settings::on_buttonBox_clicked(QAbstractButton *btn)
{
    const auto role = ui->buttonBox->buttonRole(btn);
    if (role == QDialogButtonBox::ApplyRole)
    {
        applyUiToModel();
        emit settingsApplied(_values);
    }
}

void Settings::updateColorPreview(QPushButton *btn, const QColor &c)
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

bool Settings::pickColor(QWidget *parent, const QString &title,
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

void Settings::attachColorPicker(QPushButton *btn, QColor *target)
{
    if (!btn || !target) return;
    updateColorPreview(btn, *target);
    connect(btn, &QPushButton::clicked, this, [this, btn, target]{
        pickColor(this, tr("Выбор цвета"), *target, btn);
    });
}
