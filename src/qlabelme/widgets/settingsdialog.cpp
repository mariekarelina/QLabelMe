#include "settingsdialog.h"
#include <QColorDialog>
#include <QVBoxLayout>
#include <QtWidgets>

SettingsDialog::SettingsDialog(
        qreal lineWidth,
        qreal handleSize,
        qreal numberFontPt,
        const QColor& handleColor,
        const QColor& selectedHandleColor,
        const QColor& numberColor,
        const QColor& numberBgColor,
        const QColor& rectangleLineColor,
        const QColor& circleLineColor,
        const QColor& polylineLineColor,
        QWidget* parent)
    : QDialog(parent),
        m_lineWidth(lineWidth),
        m_handleSize(handleSize),
        m_numberFontPt(numberFontPt),
        m_handleColor(handleColor),
        m_selectedHandleColor(selectedHandleColor),
        m_numberColor(numberColor),
        m_numberBgColor(numberBgColor),
        m_rectangleLineColor(rectangleLineColor),
        m_circleLineColor(circleLineColor),
        m_polylineLineColor(polylineLineColor)
{
    setWindowTitle(tr("Настройки"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowFlags(windowFlags() & ~Qt::WindowMinMaxButtonsHint);
    resize(500, 320);
    setMinimumSize(450, 260);

    auto* tabs = new QTabWidget(this);

    // Вкладка "Общее"
    auto* generalTab = new QWidget(this);
    tabs->addTab(generalTab, tr("Общее"));

    // Вкладка "Вид"
    auto* lookTab = new QWidget(this);
    auto* lookLay = new QFormLayout(lookTab);

    lookLay->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    lookLay->setLabelAlignment(Qt::AlignLeft);
    lookLay->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
    lookLay->setContentsMargins(8, 8, 8, 8);

    m_lineWidthSpin = new QDoubleSpinBox(lookTab);
    m_lineWidthSpin->setRange(0.5, 20.0);
    m_lineWidthSpin->setSingleStep(0.5);
    m_lineWidthSpin->setValue(m_lineWidth);

    m_handleSizeSpin = new QDoubleSpinBox(lookTab);
    m_handleSizeSpin->setRange(2.0, 50.0);
    m_handleSizeSpin->setSingleStep(1.0);
    m_handleSizeSpin->setValue(m_handleSize);

    m_numberFontSpin = new QDoubleSpinBox(lookTab);
    m_numberFontSpin->setRange(6.0, 72.0);
    m_numberFontSpin->setSingleStep(1.0);
    m_numberFontSpin->setValue(m_numberFontPt);

    lookLay->addRow(tr("Толщина линии"), m_lineWidthSpin);
    lookLay->addRow(tr("Размер узла"), m_handleSizeSpin);
    lookLay->addRow(tr("Размер нумерации"), m_numberFontSpin);

    tabs->addTab(lookTab, tr("Вид"));


    // Вкладка "Цвет"
    auto* colorsTab = new QWidget(this);
    auto* colorsLay = new QFormLayout(colorsTab);

    colorsLay->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    colorsLay->setLabelAlignment(Qt::AlignLeft);
    colorsLay->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
    colorsLay->setContentsMargins(8, 8, 8, 8);

    auto prepBtn = [](QPushButton* b){ b->setFixedSize(28, 28); };

    m_handleColorBtn = new QPushButton(colorsTab);
    prepBtn(m_handleColorBtn);
    m_selectedHandleColorBtn = new QPushButton(colorsTab);
    prepBtn(m_selectedHandleColorBtn);
    m_numberColorBtn = new QPushButton(colorsTab);
    prepBtn(m_numberColorBtn);
    m_numberBgColorBtn = new QPushButton(colorsTab);
    prepBtn(m_numberBgColorBtn);
    m_rectLineColorBtn = new QPushButton(colorsTab);
    prepBtn(m_rectLineColorBtn);
    m_circleLineColorBtn = new QPushButton(colorsTab);
    prepBtn(m_circleLineColorBtn);
    m_polylineLineColorBtn = new QPushButton(colorsTab);
    prepBtn(m_polylineLineColorBtn);

    paintColorButton(m_handleColorBtn, m_handleColor);
    paintColorButton(m_selectedHandleColorBtn, m_selectedHandleColor);
    paintColorButton(m_numberColorBtn, m_numberColor);
    paintColorButton(m_numberBgColorBtn, m_numberBgColor);
    paintColorButton(m_rectLineColorBtn, m_rectangleLineColor);
    paintColorButton(m_circleLineColorBtn, m_circleLineColor);
    paintColorButton(m_polylineLineColorBtn, m_polylineLineColor);

    colorsLay->addRow(tr("Цвет узла"), m_handleColorBtn);
    colorsLay->addRow(tr("Цвет выбранного узла"), m_selectedHandleColorBtn);
    colorsLay->addRow(tr("Цвет нумерации"), m_numberColorBtn);
    colorsLay->addRow(tr("Фон нумерации"), m_numberBgColorBtn);
    colorsLay->addRow(tr("Линии прямоугольника"), m_rectLineColorBtn);
    colorsLay->addRow(tr("Линии окружности"), m_circleLineColorBtn);
    colorsLay->addRow(tr("Линии полилинии"), m_polylineLineColorBtn);

    connect(m_handleColorBtn,        &QPushButton::clicked, this, [this]{ pickColor(this, tr("Цвет узла"),                m_handleColor,          m_handleColorBtn); });
    connect(m_selectedHandleColorBtn,&QPushButton::clicked, this, [this]{ pickColor(this, tr("Цвет выбранного узла"),     m_selectedHandleColor,  m_selectedHandleColorBtn); });
    connect(m_numberColorBtn,        &QPushButton::clicked, this, [this]{ pickColor(this, tr("Цвет нумерации"),           m_numberColor,          m_numberColorBtn); });
    connect(m_numberBgColorBtn,      &QPushButton::clicked, this, [this]{ pickColor(this, tr("Фон нумерации"),            m_numberBgColor,        m_numberBgColorBtn); });
    connect(m_rectLineColorBtn,      &QPushButton::clicked, this, [this]{ pickColor(this, tr("Цвет линий прямоугольника"),m_rectangleLineColor,   m_rectLineColorBtn); });
    connect(m_circleLineColorBtn,    &QPushButton::clicked, this, [this]{ pickColor(this, tr("Цвет линий окружности"),    m_circleLineColor,      m_circleLineColorBtn); });
    connect(m_polylineLineColorBtn,  &QPushButton::clicked, this, [this]{ pickColor(this, tr("Цвет линий полилинии"),     m_polylineLineColor,    m_polylineLineColorBtn); });

    tabs->addTab(colorsTab, tr("Цвет"));

    // Вкладка "Рабочая директория"
    auto* workdirTab = new QWidget(this);
    tabs->addTab(workdirTab, tr("Рабочая директория"));


    // Кнопки
    auto* box = new QDialogButtonBox(this);
    auto* okBtn     = new QPushButton(tr("ОК"));
    auto* applyBtn  = new QPushButton(tr("Применить"));
    auto* cancelBtn = new QPushButton(tr("Отмена"));
    box->addButton(okBtn,     QDialogButtonBox::AcceptRole);
    box->addButton(applyBtn,  QDialogButtonBox::ApplyRole);
    box->addButton(cancelBtn, QDialogButtonBox::RejectRole);

    connect(okBtn, &QPushButton::clicked, this, [this]{
        emit settingsApplied(this->lineWidth(), this->handleSize(), this->numberFontPt(),
                             this->handleColor(), this->selectedHandleColor(), this->numberColor(), this->numberBgColor(),
                             this->rectangleLineColor(), this->circleLineColor(), this->polylineLineColor());

        accept();
    });
    connect(applyBtn, &QPushButton::clicked, this, [this]{
        emit settingsApplied(this->lineWidth(), this->handleSize(), this->numberFontPt(),
                             this->handleColor(), this->selectedHandleColor(), this->numberColor(), this->numberBgColor(),
                             this->rectangleLineColor(), this->circleLineColor(), this->polylineLineColor());

    });
    connect(cancelBtn, &QPushButton::clicked, this, [this]{ reject(); });

    auto* root = new QVBoxLayout(this);
    root->addWidget(tabs);
    root->addWidget(box);
}

qreal SettingsDialog::lineWidth() const
{
    return m_lineWidthSpin->value();
}

qreal SettingsDialog::handleSize() const
{
    return m_handleSizeSpin->value();
}

qreal SettingsDialog::numberFontPt() const
{
    return m_numberFontSpin->value();
}

QColor SettingsDialog::handleColor() const
{
    return m_handleColor;
}

QColor SettingsDialog::selectedHandleColor() const
{
    return m_selectedHandleColor;
}

QColor SettingsDialog::numberColor() const
{
    return m_numberColor;
}

QColor SettingsDialog::numberBgColor() const
{
    return m_numberBgColor;
}

QColor SettingsDialog::rectangleLineColor() const
{
    return m_rectangleLineColor;
}

QColor SettingsDialog::circleLineColor() const
{
    return m_circleLineColor;
}

QColor SettingsDialog::polylineLineColor() const
{
    return m_polylineLineColor;
}

void SettingsDialog::paintColorButton(QPushButton* btn, const QColor& color)
{
    btn->setStyleSheet(QStringLiteral("background-color:%1; border:1px solid gray;")
                           .arg(color.name(QColor::HexArgb)));
}

bool SettingsDialog::pickColor(QWidget* parent, const QString& title, QColor& ioColor, QPushButton* previewBtn)
{
    const QColor chosen = QColorDialog::getColor(ioColor, parent, title, QColorDialog::ShowAlphaChannel);
    if (!chosen.isValid())
        return false;
    ioColor = chosen;
    paintColorButton(previewBtn, ioColor);
    return true;
}
