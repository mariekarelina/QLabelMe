#pragma once

#include <QDialog>
#include <QTabWidget>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QColor>
#include <QRadioButton>

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
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

    struct Values
    {
        int lineWidth = 2;
        int handleSize = 10;
        int numberFontPt = 12;

        QColor nodeColor = QColor("#3b6cff");
        QColor nodeSelectedColor = QColor("#ffe100");
        QColor numberColor = QColor("#ffffff");
        QColor numberBgColor = QColor("#000000");
        QColor rectLineColor = QColor("#3b6cff");
        QColor circleLineColor = QColor("#6aa7ff");
        QColor polylineLineColor = QColor("#40ff40");

        PolylineCloseMode closeMethod = PolylineCloseMode::KeyCWithoutNewPoint;
    };

    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

    void   setValues(const Values& v);
    Values values() const { return m_values; }

signals:
    void settingsApplied(const SettingsDialog::Values& v);

private slots:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();
    void on_buttonBox_clicked(QAbstractButton* btn);

private:
    void wireUi();
    void applyModelToUi();
    void applyUiToModel();
    void setupCloseMethodGroup();

    static void updateColorPreview(QPushButton *btn, const QColor &c);
    static bool pickColor(QWidget *parent, const QString &title,
                          QColor &ioColor, QPushButton *previewBtn);
    void attachColorPicker(QPushButton *btn, QColor *target);

private:
    Ui::SettingsDialog* ui = {nullptr};
    Values m_values;

    QPushButton* m_btnNode = {nullptr};
    QPushButton* m_btnNodeSel = {nullptr};
    QPushButton* m_btnNumber = {nullptr};
    QPushButton* m_btnNumberBg = {nullptr};
    QPushButton* m_btnRect = {nullptr};
    QPushButton* m_btnCircle = {nullptr};
    QPushButton* m_btnPolyline = {nullptr};
};
