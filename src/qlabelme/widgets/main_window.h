#pragma once

#include "shared/container_ptr.h"
#include "shared/qt/quuidex.h"

//#include "pproto/func_invoker.h"
//#include "pproto/transport/tcp.h"

//#include "commands/commands.h"
//#include "commands/error.h"

#include "qgraphics2/video_rect.h"
#include "qgraphics2/rectangle.h"
#include "qgraphics2/drag_circle.h"

//#include "qutils/video_widget.h"

#include "handle.h"
#include "square.h"
#include "line.h"

#include "lambda_command.h"

#include <QMainWindow>
#include <QFileDialog>
#include <QCloseEvent>
#include <QGraphicsScene>
#include <QWheelEvent>
#include <QToolButton>
#include <QListWidget>
#include <QMap>
#include <QColor>
#include <QList>
#include <QCheckBox>
#include <QButtonGroup>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidgetItem>
#include <QColorDialog>
#include <QUndoCommand>
#include <QShortcut>
#include <optional>
#include <QPointer>

#include <QUndoStack>
#include <QUndoGroup>
#include <QUndoView>
#include <QDockWidget>

#include <QJsonObject>
#include <QJsonArray>

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsLineItem>
#include <QGraphicsTextItem>
//#include "graphicsscene.h"
#include "graphics_view.h"
#include "settings.h"
#include "project_settings.h"
#include "square.h"

#include "qgraphics2/circle.h"
#include "qgraphics2/rectangle.h"
#include "qgraphics2/polyline.h"
#include "qgraphics2/point.h"
#include "qgraphics2/line.h"

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
    QGraphicsPixmapItem* pixmapItem{}; // Элемент на сцене
    bool isModified = false; // Флаг, указывающий на наличие несохраненных изменений
    std::unique_ptr<QUndoStack> _undoStack;

    // Состояние просмотра
    struct
    {
        int hScroll = 0;
        int vScroll = 0;
        // qreal zoom = 1.0;
        qreal zoom = 0.0;
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

        videoRect->setPos(0, 0);
        scene->setSceneRect(QRectF(QPointF(0, 0), QSizeF(pixmap.size())));
        return true;
    }
};

struct ShapeSnapshot
{
    QString type;      // circle/rectangle/polyline/point/line
    QVariant state;    // То, что вернул shape->saveState()
};

// Тип фигур
enum class ShapeKind {Rectangle, Circle, Polyline, Line, Point};

struct ShapeBackup
{
    ShapeKind kind;
    QString   className;
    qreal     z = 0.0;
    bool      visible = true;

    // Геометрия по типам
    QRectF rect;
    QPoint circleCenter;
    int circleRadius = 0;
    QVector<QPointF> points;
    QPoint pointCenter;

    bool closed = false;
    qulonglong uid = 0;
};

class MainWindow : public QMainWindow
{
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    bool init();
    void deinit();

    void graphicsView_mousePressEvent(QMouseEvent*, GraphicsView*);
    void graphicsView_mouseMoveEvent(QMouseEvent*, GraphicsView*);
    void graphicsView_mouseReleaseEvent(QMouseEvent*, GraphicsView*);

    void setSceneItemsMovable(bool movable);
    QColor selectedHandleColor() const {return _vis.selectedHandleColor;}
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

    void on_btnRect_clicked(bool);
    void on_btnPolyline_clicked(bool);
    void on_btnCircle_clicked(bool);
    void on_btnTest_clicked(bool);


    void fitImageToView();
    void fileList_ItemChanged(QListWidgetItem* current, QListWidgetItem* previous);
    void onPolylineModified();
    void onSceneChanged();
    void onPolygonListSelectionChanged();

    void on_actRect_triggered();
    void on_actCircle_triggered();
    void on_actPolyline_triggered();
    void on_actPoint_triggered();
    void on_actLine_triggered();
    void on_actRuler_triggered();

    void on_actClosePolyline_triggered();

    void wheelEvent(QWheelEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

    void onCheckBoxPolygonLabel(QAbstractButton *button);
    void setWorkingFolder(const QString& folderPath);

    void onSceneItemRemoved(QGraphicsItem* item);
    void on_actDelete_triggered();
    void on_actAbout_triggered();
    void on_actSettingsApp_triggered();
    void on_actSettingsProj_triggered();

    void nextImage();
    void prevImage();

    void on_actBack_triggered();
    void on_actViewNum_triggered();

    void on_actRotatePointsClockwise_triggered();
    void on_actRotatePointsCounterClockwise_triggered();

    void on_Copy_triggered();
    void on_Paste_triggered();

    void on_toggleSelectionFrame_triggered();

    void on_actUndo_triggered();
    void on_actRedo_triggered();

    void on_actResetAnnotation_triggered();
    void on_actRestoreAnnotation_triggered();

    void on_actMenu_triggered();
    void on_actFitImageToView_triggered();
    void on_actScrollBars_triggered(bool checked);

    void on_actUserGuide_triggered();

private:
    Q_OBJECT
    void loadFilesFromFolder(const QString& folderPath);

    void updatePolygonListForCurrentScene();

    void loadGeometry();
    void saveGeometry();

    // Системный буфер обмена
    void writeShapesJsonToClipboard(const QJsonObject& json) const;
    QJsonObject readShapesJsonFromClipboard() const;

    QJsonObject serializeSceneToJson(QGraphicsScene* scene);
    void deserializeJsonToScene(QGraphicsScene* scene, const QJsonObject& json);

    // Только выделенных элементов
    QJsonObject serializeSelectedItemsToJson(QGraphicsScene* scene);
    // Логика копирования/вставки между сценами
    void copySelectedShapes();
    void pasteCopiedShapesToCurrentScene();

    qgraph::VideoRect* findVideoRect(QGraphicsScene* scene);

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

    QString classesYamlPath() const;
    const QStringList& projectClasses() const {return _projectClasses;}

    bool loadClassesFromFile(const QString& filePath);
    bool saveProjectClasses(const QStringList& classes, const QMap<QString, QColor>& colors);

    void onPolygonListItemClicked(QListWidgetItem* item);
    void onPolygonListItemDoubleClicked(QListWidgetItem* item);
    void onSceneSelectionChanged();

    void updateCoordinateList();
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
    void setGhostStyleHover();  // Желтый + увеличенный
    void setGhostStyleIdle();   // Исходный размер/цвет
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
    void apply_LineColor_ToScene(QGraphicsItem* it);
    void apply_PointStyle_ToItem(QGraphicsItem* it);

    void applyLabelFontToUi();
    void updateLineColorsForScene(QGraphicsScene* scene);

    void applyZoom(qreal z);

    Settings::PolylineCloseMode _polylineCloseMode =
            Settings::PolylineCloseMode::DoubleClick;
    Settings::LineFinishMode _lineFinishMode =
            Settings::LineFinishMode::DoubleClick;

    void applyClosePolyline();
    void applyFinishLine();

    // Стек действий
    QUndoStack* activeUndoStack() const;

    // Снять снимок с произвольного QGraphicsItem
    ShapeBackup makeBackupFromItem(QGraphicsItem* gi) const;

    // Создаем снимки, по которому потом будем восстанавливать фигуры
    QVector<ShapeBackup> collectBackupsForItems(const QList<QListWidgetItem*>& listItems) const;

    // Создаем фигуру из снимка
    QGraphicsItem* recreateFromBackup(const ShapeBackup&);
    void applyBackupToExisting(QGraphicsItem*, const ShapeBackup&);

    // Привязать уже созданную фигуру к Undo/Redo
    void pushAdoptExistingShapeCommand(QGraphicsItem* createdNow,
                                       const ShapeBackup& backup,
                                       const QString& description);

    // Создание фигуры через undo/redo по заранее собранному ShapeBackup
    void pushCreateShapeCommand(const ShapeBackup& backup, const QString& description);

    // Перемещение фигур
    void pushMoveShapeCommand(QGraphicsItem* item,
                              const ShapeBackup& before,
                              const ShapeBackup& after,
                              const QString& description);

    // Для узлов
    void pushHandleEditCommand(QGraphicsItem* item,
                               const ShapeBackup& before,
                               const ShapeBackup& after,
                               const QString& description);
    // Добавление/удаление узлов у линий
    void pushModifyShapeCommand(qulonglong uid,
                                const ShapeBackup& before,
                                const ShapeBackup& after,
                                const QString& description);
    // Изменение положения изображения
    void pushMoveImageCommand(const QPointF& before,
                              const QPointF& after,
                              const QString& description);


    // Удаляет несколько фигур сразу
    void removeSceneAndListItems(const QList<QListWidgetItem*>& listItems);

    // Удаляет одну запись из списка по заданному QGraphicsItem
    void removeListEntryBySceneItem(QGraphicsItem* sceneItem);
    static QGraphicsItem* sceneItemFromListItem(const QListWidgetItem* it);
    // Поиск фигуры по uid на текущей сцене
    QGraphicsItem* findItemByUid(qulonglong uid) const;
    // Выдать/присвоить uid предмету
    qulonglong ensureUid(QGraphicsItem* it) const;
    // "Отмена" линейки
    void cancelRulerMode();
    // Видимость menuBar
    void toggleMenuBarVisible();

    QColor classColorFor(const QString& className) const;
    void applyClassColorToItem(QGraphicsItem* item, const QString& className);

    void updateImageSizeLabel(const QSize& size);

private:
    Ui::MainWindow* ui;
    static QUuidEx _applId;

    qgraph::VideoRect* _videoRect = {nullptr};

    QGraphicsScene* _scene = nullptr;
    //GraphicsView* _graphView;

    QLabel* _imageSizeLabel = nullptr; // Размер изображения в statusBar

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
    QPoint _lastMousePos; // Последняя позиция мыши


    //QToolButton* _selectModeButton;
    bool _btnRectFlag = {false};
    bool _btnPolylineFlag = {false};
    bool _btnCircleFlag = {false};
    bool _btnPointFlag = {false};
    bool _btnLineFlag = {false};


    QGraphicsItem* _draggingItem = {nullptr}; // Указатель на перетаскиваемый объект

    bool _drawingCircle = {false};    // Флаг, рисуется ли круг
    bool _drawingLine = {false};      // Флаг, рисуется ли линия
    bool _drawingPolyline = {false};  // Флаг, рисуется ли полилиния
    bool _drawingRectangle = {false}; // Флаг, рисуется ли прямоугольник
    bool _drawingPoint = {false};
    bool _isInDrawingMode = false; // Флаг, указывающий что мы в режиме рисования новой фигуры
    bool _isDraggingImage = false; // Флаг для перетаскивания изображения
    bool _isAllMoved = false; // Флаг для перетаскивания изображения вместе с разметкой
    qgraph::Polyline* _currentLine = {nullptr}; // Текущая линия
    SquareDrawingItem* _currentSquare = {nullptr}; // Текущий квадрат
    QList<QPointF> _polylinePoints; // Точки текущей полилинии


    QPointF _startPoint;

    QGraphicsRectItem* _currRectangle = {nullptr};
    bool _isDrawingRectangle = false;

    QGraphicsEllipseItem* _currCircle = {nullptr};
    bool _isDrawingCircle = false;
    // Временный крестик центра при рисовании круга
    QGraphicsLineItem* _currCircleCrossV = {nullptr};
    QGraphicsLineItem* _currCircleCrossH = {nullptr};

    QGraphicsPathItem* _currPolyline = {nullptr}; // Временная визуализация полилинии
    bool _isDrawingPolyline = false; // Флаг для состояния рисования

    bool _isDrawingPoint = false;

    qgraph::Line* _currLine = nullptr;
    bool _isDrawingLine = false;
    QPointF _lineFirstPoint;

    // Линейка
    QGraphicsLineItem* _rulerLine = nullptr; // Временная линия измерения
    QGraphicsTextItem* _rulerText = nullptr; // Подпись с расстоянием
    bool _drawingRuler = false; // Выбран ли инструмент "Линейка"
    bool _isDrawingRuler = false; // Тянем вторую точку
    QPointF _rulerStartPoint; // Первая точка измерения


    qgraph::Polyline* _polyline = {nullptr};
    qgraph::Line* _line = {nullptr};
    qgraph::Point* _currPoint = {nullptr};

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
    // Буфер для копирования фигур между сценами
    QJsonObject _shapesClipboard;

    QMap<QString, QColor> _projectClassColors;

    // Временные данные для рисования
    QGraphicsRectItem* _tempRectItem = nullptr;
    QGraphicsEllipseItem* _tempCircleItem = nullptr;
    qgraph::Polyline* _tempPolyline = nullptr;


    struct ScrollState
    {
       int hScroll = 0;
       int vScroll = 0;
       qreal zoom = 1.0;
       QPointF center;
    };
    QMap<QString, ScrollState> _scrollStates; // Ключ - путь к файлу
    // double currentScale = 1.0;
    // const double scaleStep = 1.1;
    qreal m_zoom = 1.0;
    static constexpr qreal kZoomStep = 1.1;
    static constexpr qreal kMinZoom  = 0.10;
    static constexpr qreal kMaxZoom  = 10.0;

    QList<int> _savedSplitterSizes; // Хранит нормальные размеры сплиттера
    bool _isRightSplitterCollapsed = false;
    bool _isRightPanelVisible = true;
    QWidget* _rightPanel = {nullptr}; // Указатель на правую панель

    // Указатель на последний выбранный чекбокс в списке классов для полигонов
    QCheckBox* _lastCheckedPolygonLabel = {nullptr};

    // Механика призрачной ручки
    QGraphicsRectItem* _ghostHandle = nullptr; // Рисуемая сверху копия
    qgraph::DragCircle* _ghostTarget = nullptr; // Реальная ручка
    bool _ghostActive = false;
    bool _ghostHover  = false;
    QPointF _ghostGrabOffset; // Смещение точки хвата
    qreal _ghostPickRadius = 6.0; // Радиус поиска ручки под курсором

    QVector<qgraph::DragCircle*> m_circles;
    qgraph::DragCircle* m_selectedCircle = nullptr;
    QPointF m_dragOffset;

    // Состояние перетаскивания
    bool m_isDraggingHandle = false;
    QPointer<qgraph::DragCircle> m_dragHandle; // Какую ручку тащим
    QPointF m_pressLocalOffset;                // Смещение от центра ручки при захвате

    // Состояние левой кнопки мыши над графическим видом
    bool _leftMouseButtonDown = false;

    qgraph::DragCircle* _currentDraggedCircle = nullptr;
    QPointF _dragCircleStartPosition;
    QPointF _dragCircleMouseOffset;

    bool _loadingNow = false;
    bool _editInProgress = false;
    bool _handleDragging = false; // Флаг для отслеживания перетаскивания ручек

    qgraph::DragCircle* _currentHoveredHandle = nullptr;

    qgraph::DragCircle* _lastHoverHandle = nullptr; // Кто сейчас в hover-стиле

    struct VisualStyle
    {
        qreal lineWidth = 2.0;
        qreal handleSize = 10.0;
        qreal numberFontPt = 10.0;
        int labelFontPt = 0;
        QString labelFont;
        QColor handleColor = Qt::red;
        QColor selectedHandleColor = Qt::yellow;
        QColor numberColor = Qt::white;
        QColor numberBgColor = QColor(0, 0, 0, 180);

        // Цвета линий для разных примитивов
        QColor rectangleLineColor = Qt::green;
        QColor circleLineColor = Qt::red;
        QColor polylineLineColor = Qt::blue;

        QColor lineLineColor = Qt::red;
        QColor pointColor = Qt::green;
        int pointOutlineWidth = 1;
        int pointSize = 6;

    };
    VisualStyle _vis; // Глобально для всех фигур

    //std::unique_ptr<QUndoStack> _undoStack;
    QUndoGroup* _undoGroup = nullptr;
    QUndoView* _undoView = nullptr; // Ссылка на вид из .ui
    QStringList _projectClasses;    // Единый список классов проекта
    ProjectSettings* _projPropsDialog = nullptr;

    QAction* _actUndo = {nullptr};
    QAction* _actRedo = {nullptr};

    // Перемещение фигур
    QGraphicsItem* _movingItem = nullptr;
    ShapeBackup    _moveBeforeSnap;
    bool           _moveHadChanges = false;

    QPointF        _moveGrabOffsetScene; // Смещение узла относительно item->scenePos()
    QGraphicsItem::GraphicsItemFlags  _moveSavedFlags{}; // Чтобы вернуть флаги

    // Групповое перемещение фигур
    QVector<QGraphicsItem*> _movingItems; // Все фигуры, которые двигаем вместе
    QVector<ShapeBackup> _moveGroupBefore; // Снимки фигур до перемещения
    QVector<QPointF> _moveGroupInitialPos; // Начальные позиции при захвате
    bool _moveIsGroup = false; // true, если тянем сразу несколько фигур
    QPointF _movePressScenePos; // Позиция курсора в сцене в момент захвата

    // Для узлов
    QGraphicsItem* _handleEditedItem = nullptr; // Владелец перетаскиваемой ручки
    ShapeBackup    _handleBeforeSnap;           // Снимок "до"
    bool           _handleDragHadChanges = false;

    QPointF _shiftImageBeforePos;
    bool _shiftImageDragging = false;

    bool keepImageScale = false;

    // Продолжение рисования
    bool _resumeEditing = false;
    qulonglong _resumeUid = 0;
    ShapeBackup _resumeBefore;

    // Стабильный ключ в QGraphicsItem::data(...)
    static constexpr int RoleUid = 0x1337ABCD;

    // Счетчик уникальных id на время жизни документа
    mutable qulonglong _uidCounter = 1;

    // Позволяем стороннему классу видеть все
    //friend class GraphicsView;
};

Q_DECLARE_METATYPE(Document::Ptr)

