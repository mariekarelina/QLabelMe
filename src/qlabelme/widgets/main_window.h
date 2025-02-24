#pragma once

#include "shared/simple_ptr.h"
#include "pproto/func_invoker.h"
#include "pproto/transport/tcp.h"

//#include "commands/commands.h"
//#include "commands/error.h"

#include "qgraphics2/video_rect.h"
#include "qgraphics2/rectangle.h"
//#include "qutils/video_widget.h"

#include <QMainWindow>
#include <QFileDialog>
#include <QCloseEvent>
#include <QGraphicsScene>
#include <QWheelEvent>
#include <QToolButton>

#include <QGraphicsView>
#include <QGraphicsScene>
#include "graphicsscene.h"
#include "graphics_view.h"
#include "line.h"
#include "square.h"

using namespace std;
using namespace pproto;
using namespace pproto::transport;

namespace Ui {
class MainWindow;
}

class QLabel;

class MainWindow : public QMainWindow
{
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    bool init();
    void deinit();

    void graphicsView_mousePressEvent(QMouseEvent*, GraphicsView*);
    void graphicsView_mouseMoveEvent(QMouseEvent*, GraphicsView*);
    void graphicsView_mouseReleaseEvent(QMouseEvent*, GraphicsView*);


protected:
    bool eventFilter(QObject *watched, QEvent *event);


private slots:
    void togglePointerMode();
    void on_actOpen_triggered(bool);
    void on_actClose_triggered(bool);

    void on_actCreateRectangle_triggered();
    void on_actCreateCircle_triggered();
    void on_actCreateLine_triggered();

    void on_btnRect_clicked(bool);
    void on_btnLine_clicked(bool);
    void on_btnCircle_clicked(bool);
    void on_btnTest_clicked(bool);

private:
    Q_OBJECT
    void closeEvent(QCloseEvent*) override;

    void loadGeometry();
    void saveGeometry();
//    void zoomIn();
//    void zoomOut();
//    void zoomInPos(QPointF scenePos);
//    void zoomOutPos(QPointF scenePos);

    void graphicShapeChange(QGraphicsItem* item);


private:
    // Члены класса начинаются с _
    Ui::MainWindow* ui;
    static QUuidEx _applId;

    qgraph::VideoRect* _videoRect = {nullptr};


    QGraphicsScene* _scene = nullptr;
    //GraphicsView* _graphView;

    QString _windowTitle;
    QLabel* _labelConnectStatus;

    // Признак UltraHD монитора
    bool _ultraHD = {false};

    QPoint _point;
    QList <QPointF> _points;

    bool _dragging = false; // Флаг для отслеживания состояния перетаскивания
    QPoint _lastMousePos;   // Последняя позиция мыши
    QVector<QPointF> _stuff;
    QPointF _scenePos;

    QLabel* label;
    QLabel* label2;
    QToolButton* selectModeButton;
    bool _btnRectFlag = {false};
    bool _btnLineFlag = {false};
    bool _btnCircleFlag = {false};

    QGraphicsItem* _draggingItem = {nullptr}; // Указатель на перетаскиваемый объект


    bool drawingLine = {false};      // Флаг, рисуется ли линия
    bool drawingSquare = {false};    // Флаг, рисуется ли квадрат
    LineDrawingItem* currentLine = {nullptr}; // Текущая линия
    SquareDrawingItem* currentSquare = {nullptr}; // Текущий квадрат

    // Позволяем стороннему классу видеть все
    //friend class GraphicsView;
};

