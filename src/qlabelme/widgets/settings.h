#pragma once

#include <QDialog>
#include <QTabWidget>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QColor>
#include <QRadioButton>
#include <QFont>

namespace Ui {
class Settings;
}

class QShowEvent;

class Settings : public QDialog
{
    Q_OBJECT
public:

    enum class PolylineCloseMode
    {
        DoubleClick = 0,
        SingleClickOnFirstPoint = 1,
        CtrlModifier = 2,
        KeyCWithoutNewPoint = 3
    };
    enum class LineFinishMode
    {
        DoubleClick = 0,
        CtrlModifier = 1,
        KeyCWithoutNewPoint = 2
    };

    struct Values
    {
        int lineWidth = 2;
        int handleSize = 10;
        int numberFontPt = 12;
        int labelFontPt = 12;
        QString labelFont;

        QColor nodeColor = QColor("#3b6cff");
        QColor nodeSelectedColor = QColor("#ffe100");
        QColor numberColor = QColor("#ffffff");
        QColor numberBgColor = QColor("#000000");
        QColor rectLineColor = QColor("#3b6cff");
        QColor circleLineColor = QColor("#6aa7ff");
        QColor polylineLineColor = QColor("#40ff40");
        QColor lineLineColor = QColor("#40ff40");
        QColor pointColor = QColor("#00ff00");
        int pointOutlineWidth = 1;
        int pointSize = 6;
        bool keepImageScale = false;
        bool keepMenuBarVisibility = false;
        int handlePickRadius = 6; // Область захвата узла

        PolylineCloseMode closePolyline = PolylineCloseMode::DoubleClick;
        LineFinishMode finishLine = LineFinishMode::DoubleClick;
    };

    explicit Settings(QWidget *parent = nullptr);
    ~Settings();

    void   setValues(const Values& v);
    Values values() const { return _values; }

protected:
    void showEvent(QShowEvent* e) override;

signals:
    void settingsApplied(const Settings::Values& v);

private slots:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();
    void on_buttonBox_clicked(QAbstractButton* btn);
    void onPickLabelFont();

private:
    void wireUi();
    void applyModelToUi();
    void applyUiToModel();
    void setupCloseMethodGroup();
    void setupFinishMethodGroup();

    static void updateColorPreview(QPushButton *btn, const QColor &c);
    static bool pickColor(QWidget *parent, const QString &title,
                          QColor &ioColor, QPushButton *previewBtn);
    void attachColorPicker(QPushButton *btn, QColor *target);
    void updateLabelFontPreview();

private:
    Ui::Settings* ui = {nullptr};   
    Values _values;

    QPushButton* _btnNode = {nullptr};
    QPushButton* _btnNodeSel = {nullptr};
    QPushButton* _btnNumber = {nullptr};
    QPushButton* _btnNumberBg = {nullptr};
    QPushButton* _btnRect = {nullptr};
    QPushButton* _btnCircle = {nullptr};
    QPushButton* _btnPolyline = {nullptr};
    QPushButton* _btnLine = {nullptr};
    QPushButton* _btnPoint = {nullptr};

    QFont _labelFont; // Выбранный шрифт
};
