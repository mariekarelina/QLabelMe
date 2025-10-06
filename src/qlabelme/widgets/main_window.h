#pragma once

#include "shared/container_ptr.h"
//#include "shared/simple_ptr.h"

//#include "pproto/func_invoker.h"
#include "pproto/transport/tcp.h"

//#include "commands/commands.h"
//#include "commands/error.h"

#include "qgraphics2/video_rect.h"
#include "qgraphics2/rectangle.h"
#include "qgraphics2/drag_circle.h"

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
#include <QCheckBox>
#include <QButtonGroup>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidgetItem>
#include <QColorDialog>
#include <QUndoStack>
#include <QUndoCommand>
#include <QShortcut>
#include <QPointer>

#include <QGraphicsView>
#include <QGraphicsScene>
//#include "graphicsscene.h"
#include "graphics_view.h"
#include "line.h"
#include "square.h"

#include "qgraphics2/circle.h"
#include "qgraphics2/rectangle.h"
#include "qgraphics2/polyline.h"

// using namespace std;
// using namespace pproto;
// using namespace pproto::transport;

namespace Ui {
class MainWindow;
}

class QLabel;

struct Document
{
    typedef container_ptr<Document> Ptr;

    QString filePath;  // Путь к файлу изображения
    QGraphicsScene* scene = nullptr;  // Сцена с изображением и разметкой
    qgraph::VideoRect* videoRect = nullptr;
    QPixmap pixmap;  // Само изображение
    bool isModified = false; // Флаг, указывающий на наличие несохраненных изменений

    // Состояние просмотра
    struct
    {
        int hScroll = 0;
        int vScroll = 0;
        qreal zoom = 1.0;
        QPointF center;
    } viewState;

    static Ptr create(const QString& path)
    {
        Ptr doc {new Document};
        doc->filePath = path;
        return doc;
    }

    bool loadImage()
    {
        pixmap = QPixmap(filePath);
        if (pixmap.isNull())
        {
            return false;
        }
        if (!scene)
        {
            scene = new QGraphicsScene();
        }
        if (!videoRect)
        {
            videoRect = new qgraph::VideoRect(scene);
        }
        videoRect->setPixmap(pixmap);
        return true;
    }
};

// struct ShapeSnapshot
// {
//     QString type;
//     QVariant state;
//     int zvalue = 0;
//     bool visible = true;
//     QPointF pos;
// };
struct ShapeSnapshot
{
    QString type;      // circle/rectangle/polyline/...
    QVariant state;    // то, что вернул shape->saveState()
};

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
    QColor selectedHandleColor() const { return _vis.selectedHandleColor; }
    Document::Ptr currentDocument() const;

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void closeEvent(QCloseEvent*) override;

private slots:
    void togglePointerMode();
    void on_actOpen_triggered(bool);
    void on_actClose_triggered(bool);
    void on_actSave_triggered(bool);

    void on_actExit_triggered(bool);

    void on_actCreateRectangle_triggered();
    void on_actCreateCircle_triggered();
    void on_actCreateLine_triggered();

    void on_btnRect_clicked(bool);
    void on_btnLine_clicked(bool);
    void on_btnCircle_clicked(bool);
    void on_btnTest_clicked(bool);

    void fitImageToView();
    void fileList_ItemChanged(QListWidgetItem* current, QListWidgetItem* previous);
    void onPolylineModified();
    void onSceneChanged();
    void onPolygonListSelectionChanged();

    void on_actRect_triggered();
    void on_actCircle_triggered();
    void on_actLine_triggered();

    void wheelEvent(QWheelEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

    void onCheckBoxPolygonLabel(QAbstractButton *button);
    void setWorkingFolder(const QString& folderPath);

    void onSceneItemRemoved(QGraphicsItem* item);
    void on_actDelete_triggered();
    void on_actAbout_triggered();

    // Настройки
    void on_actSetting_triggered();

private:
    Q_OBJECT
    void loadFilesFromFolder(const QString& folderPath);

    void updatePolygonListForCurrentScene();

    void loadGeometry();
    void saveGeometry();

    QJsonObject serializeSceneToJson(QGraphicsScene* scene);
    void deserializeJsonToScene(QGraphicsScene* scene, const QJsonObject& json);
    qgraph::VideoRect* findVideoRect(QGraphicsScene* scene);
    //void resetViewToDefault();

    void toggleRightSplitter();
    void updateWindowTitle();
    void updateFolderPathDisplay();

    static QString annotationPathFor(const QString& imagePath);
    void saveAnnotationToFile(Document::Ptr doc);
    void updateFileListDisplay(const QString& filePath);
    void loadAnnotationFromFile(Document::Ptr doc);
    void saveCurrentViewState(Document::Ptr doc);
    void restoreViewState(Document::Ptr doc);

    void handleCheckBoxClick(QCheckBox* clickedCheckBox);

    void loadLastUsedFolder();
    void saveLastUsedFolder();

    bool hasAnnotationFile(const QString& imagePath) const;

    bool loadClassesFromFile(const QString& filePath);

    void onPolygonListItemClicked(QListWidgetItem* item);
    void onPolygonListItemDoubleClicked(QListWidgetItem* item);
    void onSceneSelectionChanged();

    void updateFileListItemIcon(QListWidgetItem* item, bool hasAnnotations);
    QListWidgetItem* findFileListItem(const QString& filePath);

    // Удаление элемента со сцены и из списка
    void removePolygonItem(QGraphicsItem* item);
    void removePolygonListItem(QListWidgetItem* item);

    // Связывание элементов сцены и списка
    void linkSceneItemToList(QGraphicsItem* sceneItem);

    void showPolygonListContextMenu(const QPoint &pos);

    bool isYamlFileEmpty(const QString& yamlPath) const;
    QString getAnnotationFilePath(const QString& imagePath) const;

    void raiseAllHandlesToTop();
    void moveSelectedItemsToBack();

    bool hasUnsavedChanges() const;
    QList<Document::Ptr> getUnsavedDocuments() const;
    int showUnsavedChangesDialog(const QList<Document::Ptr> &unsavedDocs);
    void saveAllDocuments();

    qgraph::DragCircle* pickHiddenHandle(const QPointF& scenePos, bool& topIsHandle) const;
    void ensureGhostAllocated();
    void showGhostOver(qgraph::DragCircle* target, const QPointF& scenePos);
    void moveGhostTo(const QPointF& scenePos);
    void endGhost();
    // Cтили призрака
    void setGhostStyleHover();  // жёлтый + увеличенный
    void setGhostStyleIdle();   // исходный размер/цвет
    void showGhostPreview(qgraph::DragCircle* target, const QPointF& scenePos);
    void startGhostDrag(const QPointF& scenePos);

    qgraph::DragCircle* pickHandleAt(const QPointF& scenePos) const;
    void startHandleDrag(qgraph::DragCircle* h, const QPointF& scenePos);
    void updateHandleDrag(const QPointF& scenePos);
    void finishHandleDrag();

    void clearAllHandleHoverEffects();
    void showGhostFor(qgraph::DragCircle* h);
    void hideGhost();

    void updateAllPointNumbers();


    void loadVisualStyle();
    void saveVisualStyle() const;

    void applyStyle_AllDocuments();
    void forEachScene(std::function<void(QGraphicsScene*)> fn);
    void apply_LineWidth_ToScene(QGraphicsScene* sc);
    void apply_PointSize_ToScene(QGraphicsScene* sc);
    void apply_NumberSize_ToScene(QGraphicsScene* sc);

    void apply_LineWidth_ToItem(QGraphicsItem* it);
    void apply_PointSize_ToItem(QGraphicsItem* it);
    void apply_NumberSize_ToItem(QGraphicsItem* it);

    void updateLineColorsForScene(QGraphicsScene* scene);

    void applyZoom(qreal z);

private:
    Ui::MainWindow* ui;
    static QUuidEx _applId;

    qgraph::VideoRect* _videoRect = {nullptr};

    QGraphicsScene* _scene = nullptr;
    //GraphicsView* _graphView;

    QString _windowTitle;
    QString _currentFolderPath; // Переменную для хранения пути
    QString _lastUsedFolder; // Последняя открытая папка
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
    bool _isDraggingImage = false; // Флаг для перетаскивания изображения
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

    //QMap<QString, QGraphicsScene*> _scenesMap; // Ключ - путь к файлу, значение - сцена
    QMap<QString, Document::Ptr> _documentsMap;

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
    // double currentScale = 1.0;
    // const double scaleStep = 1.1;
    qreal m_zoom = 1.0;
    static constexpr qreal kZoomStep = 1.1;
    static constexpr qreal kMinZoom  = 0.10;
    static constexpr qreal kMaxZoom  = 10.0;

    QList<int> _savedSplitterSizes; // Хранит нормальные размеры сплиттера
    bool _isRightSplitterCollapsed = false;
    bool _isRightPanelVisible = true;
    QWidget* _rightPanel; // Указатель на правую панель

    // Указатель на последний выбранный чекбокс в списке классов для полигонов
    QCheckBox* _lastCheckedPolygonLabel = nullptr;

    // --- Механика "призрачной" ручки ---
    QGraphicsRectItem* _ghostHandle = nullptr;         // рисуемая сверху копия
    qgraph::DragCircle* _ghostTarget = nullptr;        // настоящая цель (реальная ручка)
    bool _ghostActive = false;
    bool _ghostHover  = false;
    QPointF _ghostGrabOffset;                          // смещение точки хвата
    qreal _ghostPickRadius = 6.0;                     // радиус поиска ручки под курсором

    QVector<qgraph::DragCircle*> m_circles;
    qgraph::DragCircle* m_selectedCircle = nullptr;
    QPointF m_dragOffset;

    // Состояние перетаскивания
    bool m_isDraggingHandle = false;
    QPointer<qgraph::DragCircle> m_dragHandle;   // какую ручку тащим
    QPointF m_pressLocalOffset;                  // смещение от центра ручки при захвате


    qgraph::DragCircle* _currentDraggedCircle = nullptr;
    QPointF _dragCircleStartPosition;
    QPointF _dragCircleMouseOffset;

    bool _loadingNow = false;
    bool _editInProgress = false;
    bool _handleDragging = false; // Флаг для отслеживания перетаскивания ручек

    qgraph::DragCircle* _currentHoveredHandle = nullptr;

    qgraph::DragCircle* _lastHoverHandle = nullptr; // кто сейчас в hover-стиле

    struct VisualStyle
    {
        qreal lineWidth = 2.0;
        qreal handleSize = 10.0;
        qreal numberFontPt = 10.0;
        QColor handleColor = Qt::red;
        QColor selectedHandleColor = Qt::yellow;
        QColor numberColor = Qt::white;
        QColor numberBgColor = QColor(0, 0, 0, 180);

        // Цвета линий для разных примитивов
        QColor rectangleLineColor = Qt::green;
        QColor circleLineColor = Qt::red;
        QColor polylineLineColor = Qt::blue;
    };
    VisualStyle _vis; // глобально для всех фигур

    // Позволяем стороннему классу видеть все
    //friend class GraphicsView;
};

Q_DECLARE_METATYPE(Document::Ptr)

