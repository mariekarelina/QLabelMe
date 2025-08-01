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
#include <QListWidget>
#include <QMap>
#include <QList>

#include <QGraphicsView>
#include <QGraphicsScene>
#include "graphicsscene.h"
#include "graphics_view.h"
#include "line.h"
#include "square.h"

#include "selectiondialog.h"

#include "qgraphics2/circle.h"
#include "qgraphics2/rectangle.h"
#include "qgraphics2/polyline.h"

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

    void setSceneItemsMovable(bool movable);

protected:
    bool eventFilter(QObject *watched, QEvent *event);
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void togglePointerMode();
    void on_actOpen_triggered(bool);
    void on_actClose_triggered(bool);

    void on_actExit_triggered(bool);

    void on_actCreateRectangle_triggered();
    void on_actCreateCircle_triggered();
    void on_actCreateLine_triggered();

    void on_btnRect_clicked(bool);
    void on_btnLine_clicked(bool);
    void on_btnCircle_clicked(bool);
    void on_btnTest_clicked(bool);
    void saveCurrentViewState();
    void restoreViewState(const QString& filePath);
    void fitImageToView();
    //void fitImageToView(bool);
    void fileList_ItemChanged(QListWidgetItem* current, QListWidgetItem* previous);

    void on_actRect_triggered();
    void on_actCircle_triggered();
    void on_actLine_triggered();

    void wheelEvent(QWheelEvent* event);

    void resizeEvent(QResizeEvent* event);


private:
    Q_OBJECT
    void closeEvent(QCloseEvent*) override;
    void loadFilesFromFolder(const QString& folderPath);
    void loadPolygonLabelsFromFile(const QString& filePath);

    // Обновляет список разметок для текущей сцены
    void updatePolygonListForCurrentScene();

    void loadGeometry();
    void saveGeometry();

    void saveAnnotationToFile(const QString& imagePath);
    void loadAnnotationFromFile(const QString& imagePath);
    QJsonObject serializeSceneToJson(QGraphicsScene* scene);
    void deserializeJsonToScene(QGraphicsScene* scene, const QJsonObject& json);
    qgraph::VideoRect* findVideoRect(QGraphicsScene* scene);
    void resetViewToDefault();

    void toggleRightSplitter(); // Функция переключения сплиттера
    void updateWindowTitle(); // Обновление заголовка (путь к папке с файлами)
    void updateFolderPathDisplay();


private:
    Ui::MainWindow* ui;
    static QUuidEx _applId;

    qgraph::VideoRect* _videoRect = {nullptr};

    QGraphicsScene* _scene = nullptr;
    //GraphicsView* _graphView;

    QString _windowTitle;
    QString _currentFolderPath; // Переменную для хранения пути
    QLabel* _labelConnectStatus;

    QLabel* _folderPathLabel; // Для отображения пути к папке рядом с версией

    // Признак UltraHD монитора
    bool _ultraHD = {false};

    QPoint _point;
    QList <QPointF> _points;

    bool _dragging = false; // Флаг для отслеживания состояния перетаскивания
    QPoint _lastMousePos;   // Последняя позиция мыши


    //QToolButton* _selectModeButton;
    bool _btnRectFlag = {false};
    bool _btnLineFlag = {false};
    bool _btnCircleFlag = {false};

    QGraphicsItem* _draggingItem = {nullptr}; // Указатель на перетаскиваемый объект

    bool _drawingCircle = {false};      // Флаг, рисуется ли круг
    bool _drawingLine = {false};      // Флаг, рисуется ли линия
    bool _drawingPolyline = {false};      // Флаг, рисуется ли линия
    bool _drawingRectangle = {false};    // Флаг, рисуется ли прямоугольник
    bool _isInDrawingMode = false; // Флаг, указывающий что мы в режиме рисования новой фигуры
    LineDrawingItem* _currentLine = {nullptr}; // Текущая линия
    SquareDrawingItem* _currentSquare = {nullptr}; // Текущий квадрат
    QList<QPointF> _polylinePoints;            // Точки текущей полилинии


    QPointF _startPoint;

    QGraphicsRectItem* _currRectangle = {nullptr};
    bool _isDrawingRectangle = false;

    QGraphicsEllipseItem* _currCircle = {nullptr};
    bool _isDrawingCircle = false;

    QGraphicsPathItem* _currPolyline = {nullptr}; // Временная визуализация полилинии
    bool _isDrawingPolyline = false;           // Флаг для состояния рисования


    qgraph::Polyline* _polyline = {nullptr};

    struct ImageData
    {
        QGraphicsScene* scene;
        QPixmap pixmap;
        QList<QGraphicsItem*> shapes;
    };
    QMap<QString, ImageData> _imageDataMap; // Ключ - путь к файлу

    QMap<QString, QGraphicsScene*> _scenesMap; // Ключ - путь к файлу, значение - сцена

    // Текущее изображение
   QString _currentImagePath;

   // Временные данные для рисования
   QGraphicsRectItem* _tempRectItem = nullptr;
   QGraphicsEllipseItem* _tempCircleItem = nullptr;
   qgraph::Polyline* _tempPolyline = nullptr;


   struct ScrollState
   {
       int hScroll = 0;
       int vScroll = 0;
       qreal zoom = 1.0;
       QPointF center; // Центр viewport'а (дополнительно для точного восстановления)
   };
   QMap<QString, ScrollState> _scrollStates; // Ключ — путь к файлу

   QList<int> _savedSplitterSizes; // Хранит нормальные размеры сплиттера
   bool _isRightSplitterCollapsed = false;
   bool _isRightPanelVisible = true;
   QWidget* _rightPanel; // Указатель на правую панель



    // Позволяем стороннему классу видеть все
    //friend class GraphicsView;
};

