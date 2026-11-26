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
public:
    enum class PolylineCloseMode
    {
        DoubleClick = 0,
        SingleClickOnFirstPoint = 1,
        CtrlModifier = 2,
        KeyCWithoutNewPoint = 3,
        KeyEnterWithoutNewPoint = 4
    };
    enum class LineFinishMode
    {
        DoubleClick = 0,
        CtrlModifier = 1,
        KeyCWithoutNewPoint = 2,
        KeyEnterWithoutNewPoint = 3
    };

    struct Values
    {
        int lineWidth = 2;
        int handleSize = 10;
        int numberFontPt = 12;
        int labelFontPt = 12;
        QString labelFont;

        QColor nodeColor = QColor("#BAC8D6");
        QColor nodeSelectedColor = QColor("#E6B85C");
        QColor numberColor = QColor("#FFFFFF");
        QColor numberBgColor = QColor("#B4000000");
        QColor rectLineColor = QColor("#55B5AA");
        QColor circleLineColor = QColor("#AA8BD0");
        QColor polylineLineColor = QColor("#6F9ED4");
        QColor lineLineColor = QColor("#D3AC6B");
        QColor pointColor = QColor("#D98282");
        QColor rulerColor = QColor("#C8CDD3");
        int pointOutlineWidth = 1;
        int pointSize = 6;
        bool keepImageScale = false;
        bool keepMenuBarVisibility = false;
        bool autoSaveEnabled = false;
        int autoSaveIntervalSec = 30;
        double handlePickRadius = 6.0; // Область захвата узла
        double edgePickRadius = 8.0;  // Область захвата ребра
        bool showNumbers = true; // Глобальная видимость нумерации
        bool showSelectionFrame = true; // Глобальная видимость рамки фигур
        bool fillShapeWhenSelected = true; // Заполнять фигуру при выделении

        PolylineCloseMode closePolyline = PolylineCloseMode::DoubleClick;
        LineFinishMode finishLine = LineFinishMode::DoubleClick;
    };

    explicit Settings(QWidget* parent = nullptr);
    ~Settings();

    Values values() const {return _values;}
    void setValues(const Values& v);

    void loadGeometry();
    void saveGeometry() const;

signals:
    void settingsApplied(const Settings::Values& v);

private slots:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();
    void on_buttonBox_clicked(QAbstractButton* btn);
    void onPickLabelFont();

private:
    Q_OBJECT
    void showEvent(QShowEvent* e) override;

    void wireUi();
    void applyModelToUi();
    void applyUiToModel();
    void setupCloseMethodGroup();
    void setupFinishMethodGroup();


    static void updateColorPreview(QPushButton* btn, const QColor &c);
    static bool pickColor(QWidget* parent, const QString &title,
                          QColor &ioColor, QPushButton* previewBtn);
    void attachColorPicker(QPushButton* btn, QColor* target);
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
    QPushButton* _btnRuler = {nullptr};

    QFont _labelFont; // Выбранный шрифт
};
