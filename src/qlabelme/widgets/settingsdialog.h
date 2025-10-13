#pragma once

#include <QDialog>
#include <QTabWidget>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QColor>
#include <QRadioButton>

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(
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
        QWidget* parent = nullptr);

    qreal lineWidth() const;
    qreal handleSize() const;
    qreal numberFontPt() const;

    QColor handleColor() const;
    QColor selectedHandleColor() const;
    QColor numberColor() const;
    QColor numberBgColor() const;

    QColor rectangleLineColor() const;
    QColor circleLineColor() const;
    QColor polylineLineColor() const;

    enum class PolylineCloseMode
    {
        DoubleClickOnAnyPoint = 0,
        SingleClickOnFirstPoint = 1,
        CtrlModifier = 2,
        KeyCWithoutNewPoint = 3
    };

    PolylineCloseMode polylineCloseMode() const;
    void setPolylineCloseMode(PolylineCloseMode m);

signals:
    // Для кнопки «Применить»
    void settingsApplied(
        qreal lineWidth,
        qreal handleSize,
        qreal numberFontPt,
        const QColor& handleColor,
        const QColor& selectedHandleColor,
        const QColor& numberColor,
        const QColor& numberBgColor,
        const QColor& rectangleLineColor,
        const QColor& circleLineColor,
        const QColor& polylineLineColor);

private:
    static void paintColorButton(QPushButton* btn, const QColor& color);
    static bool pickColor(QWidget* parent, const QString& title, QColor& ioColor, QPushButton* previewBtn);

    qreal m_lineWidth;
    qreal m_handleSize;
    qreal m_numberFontPt;

    QColor m_handleColor;
    QColor m_selectedHandleColor;
    QColor m_numberColor;
    QColor m_numberBgColor;

    QColor m_rectangleLineColor;
    QColor m_circleLineColor;
    QColor m_polylineLineColor;

private:
    QDoubleSpinBox* m_lineWidthSpin = {nullptr};
    QDoubleSpinBox* m_handleSizeSpin = {nullptr};
    QDoubleSpinBox* m_numberFontSpin = {nullptr};

    QPushButton* m_handleColorBtn = {nullptr};
    QPushButton* m_selectedHandleColorBtn = {nullptr};
    QPushButton* m_numberColorBtn = {nullptr};
    QPushButton* m_numberBgColorBtn = {nullptr};
    QPushButton* m_rectLineColorBtn = {nullptr};
    QPushButton* m_circleLineColorBtn = {nullptr};
    QPushButton* m_polylineLineColorBtn = {nullptr};

    QWidget* tabPolylineClose = {nullptr};
    QButtonGroup* polylineCloseGroup = {nullptr};
    QRadioButton* rbDblOnAny = {nullptr};
    QRadioButton* rbSingleOnFirst = {nullptr};
    QRadioButton* rbCtrl = {nullptr};
    QRadioButton* rbKeyC = {nullptr};
};
