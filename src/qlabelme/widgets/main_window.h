#pragma once

#include "shared/config/yaml_config.h"
#include "shared/container_ptr.h"
#include "shared/qt/quuidex.h"
#include "shared/defmac.h"

#include "qgraphics2/drag_circle.h"
#include "qgraphics2/video_rect.h"

#include <QtCore>
#include <QtGui>

#include <QItemSelectionModel>
#include <QStandardItemModel>
#include <QGraphicsLineItem>
#include <QGraphicsTextItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QStandardItem>
#include <QModelIndex>
#include <QMainWindow>
#include <QUndoGroup>
#include <QUndoView>
#include <QShortcut>

#include <functional>

#include "project_settings.h"
#include "graphics_view.h"
#include "document.h"
#include "settings.h"
#include "square.h"

#include "qgraphics2/rectangle.h"
#include "qgraphics2/polyline.h"
#include "qgraphics2/circle.h"
#include "qgraphics2/point.h"
#include "qgraphics2/line.h"

#include "shared/steady_timer.h"
#include "shared/logger/logger.h"
#define log_debug2_m  alog::logger().debug2  (alog_line_location, "MainWin")


namespace Ui {
class MainWindow;
}

class QLabel;

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
    QColor selectedHandleColor() const {return _vstyle.selectedHandleColor;}
    Document::Ptr currentDocument() const;

public slots:
    void changeClassByUid(qulonglong uid);
    void toggleSceneItemVisibility(QGraphicsItem* item);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void closeEvent(QCloseEvent*) override;
    void showEvent(QShowEvent* event) override;

private slots:
    void on_actOpen_triggered(bool);
    void on_actClose_triggered(bool);
    void on_actSave_triggered(bool);

    void on_actExit_triggered(bool);

    void fitImageToView();
    void fileList_ItemChanged(QListWidgetItem* current, QListWidgetItem* previous);

    void onSceneChanged();
    void onPolygonListSelectionChanged();
    void selectAllShapes();

    void on_actRect_triggered();
    void on_actCircle_triggered();
    void on_actPolyline_triggered();
    void on_actPoint_triggered();
    void on_actLine_triggered();
    void on_actRuler_triggered();

    void on_actClosePolyline_triggered();

    void wheelEvent(QWheelEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

    void setWorkingFolder(const QString& folderPath);

    void on_actDelete_triggered();
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

private:
    Q_OBJECT
    void loadFilesFromFolder(const QString& folderPath);

    void updatePolygonListForCurrentScene();
    void changeClassForSceneItem(QGraphicsItem* item);

    void loadGeometry();
    void saveGeometry();

    // Системный буфер обмена
    QString readShapesYamlFromClipboard() const;
    QString serializeSelectedItemsToYaml(Document::Ptr doc);
    void writeShapesYamlToClipboard(const QString& yaml) const;

    // Логика копирования/вставки между сценами
    void copySelectedShapes();
    void pasteCopiedShapesToCurrentScene();

    void toggleRightSplitter();
    void updateWindowTitle();

    static QString annotationPathFor(const QString& imagePath);
    static double round2(double value);

    // Формирует YAML-разметку из переданного списка фигур
    void saveShapesToYaml(YamlConfig& yconfig,
                          const QList<QGraphicsItem*>& items);
    // Добавляет фигуры из YAML на сцену с указанным сдвигом координат
    bool loadShapesFromYaml(Document::Ptr doc,
                            YamlConfig& yconfig,
                            const QPointF& imageOffset);

    void saveAnnotationToFile(Document::Ptr doc);
    void updateFileListDisplay(const QString& filePath);
    void loadAnnotationFromFile(Document::Ptr doc, bool rebuildUi = true);

    void saveCurrentViewState(Document::Ptr doc);
    void restoreViewState(Document::Ptr doc);

    void loadLastUsedFolder();
    void saveLastUsedFolder();

    bool hasAnnotationFile(const QString& imagePath) const;

    QString classesYamlPath() const;
    const QStringList& projectClasses() const {return _projectClasses;}

    bool loadClassesFromFile(const QString& filePath);
    bool saveProjectClasses(const QStringList& classes, const QMap<QString, QColor>& colors);

    void onPolygonListItemClicked(const QModelIndex& index);
    void onPolygonListItemDoubleClicked(const QModelIndex& index);
    void onSceneSelectionChanged();

    void updateCoordinateList();

    // Связывание элементов сцены и списка
    void linkSceneItemToList(QGraphicsItem* sceneItem);
    void linkSceneItemToList(QGraphicsItem* sceneItem, int row, bool needRenumber = true);
    void renumberPolygonList();
    void renumberPolygonListTextOnly();

    void showPolygonListContextMenu(const QPoint &pos);

    void updatePolygonListItemText(int row);
    void updatePolygonListTexts();

    bool isYamlFileEmpty(const QString& yamlPath) const;
    QString getAnnotationFilePath(const QString& imagePath) const;

    void raiseAllHandlesToTop();

    bool isRootShapeItem(QGraphicsItem* item) const;
    void restoreTemporaryRaisedZValues();
    void raiseSelectedShapesTemporarily();

    void moveSelectedItemsToBack();

    bool hasUnsavedChanges() const;
    QList<Document::Ptr> getUnsavedDocuments() const;
    int showUnsavedChangesDialog(const QList<Document::Ptr>& unsavedDocs);
    void saveAllDocuments();

    qgraph::DragCircle* pickHiddenHandle(const QPointF& scenePos, bool& topIsHandle) const;
    void ensureGhostAllocated();
    void moveGhostTo(const QPointF& scenePos);
    void endGhost();
    // Cтили призрака
    void setGhostStyleHover();  // Желтый + увеличенный
    void setGhostStyleIdle();   // Исходный размер/цвет
    void startGhostDrag(const QPointF& scenePos);

    // Масштабируемые области захвата
    qreal imagePickScale() const;
    qreal viewPixelsToSceneRadius(GraphicsView* view, qreal viewPx) const;
    qreal effectiveViewPickPx(qreal baseViewPx) const;

    qgraph::DragCircle* pickHandleAt(const QPointF& scenePos) const;
    qgraph::DragCircle* pickHandleAt(const QPointF& scenePos, qreal customRadius) const;
    QGraphicsItem* pickItemByEdgeAt(GraphicsView* view, const QPoint& viewPos) const;

    void startHandleEditState(qgraph::DragCircle* handle);
    void pushHandleEditCommandToStack();

    void startHandleDrag(qgraph::DragCircle* handle, const QPointF& scenePos);
    void updateHandleDrag(const QPointF& scenePos);
    void finishHandleDrag();

    void clearAllHandleHoverEffects();
    void showGhostFor(qgraph::DragCircle* handle);
    void hideGhost();

    void updateAllPointNumbers();

    void loadVisualStyle();
    void saveVisualStyle() const;

    void applyStyle_AllDocuments();
    void forEachScene(std::function<void(QGraphicsScene*)> sceneHandler);
    void apply_LineWidth_ToScene(QGraphicsScene* scene);
    void apply_PointSize_ToScene(QGraphicsScene* scene);
    void apply_NumberSize_ToScene(QGraphicsScene* scene);

    void apply_LineWidth_ToItem(QGraphicsItem* item);
    void apply_PointSize_ToItem(QGraphicsItem* item);
    void apply_NumberSize_ToItem(QGraphicsItem* item);
    void apply_PointStyle_ToItem(QGraphicsItem* item);

    void applyLabelFontToUi();
    void updateLineColorsForScene(QGraphicsScene* scene);

    Settings::PolylineCloseMode _polylineCloseMode =
            Settings::PolylineCloseMode::DoubleClick;
    Settings::LineFinishMode _lineFinishMode =
            Settings::LineFinishMode::DoubleClick;

    void applyClosePolyline();
    void applyFinishLine();

    // Стек действий
    QUndoStack* activeUndoStack() const;
    void restoreDrawingStateAfterStackChange();

    void setPolygonListModelForCurrentDocument();

    // Удаляет одну запись из списка по заданному QGraphicsItem
    void removeListEntryBySceneItem(QGraphicsItem* sceneItem);

    QGraphicsItem* sceneItemFromListIndex(const QModelIndex& index) const;
    QGraphicsItem* sceneItemFromListRow(int row) const;
    int polygonListRowByItem(QGraphicsItem* sceneItem) const;

    // Поиск фигуры по uid на текущей сцене
    QGraphicsItem* findItemByUid(qulonglong uid) const;

    // Выдать/присвоить uid предмету
    qulonglong ensureUid(QGraphicsItem* item) const;

    // Сбросить состояния рисования при удалении line/polyline из сцены
    void clearLinePolylineStateForDeletedItem(QGraphicsItem* item);

    // Отмена линейки
    void cancelRulerMode();

    // Видимость menuBar
    void toggleMenuBarVisible();

    QColor classColorFor(const QString& className) const;
    void applyClassColorToItem(QGraphicsItem* item, const QString& className);
    void applyClassRenames(const QMap<QString, QString>& renames);

    void updateImageSizeLabel(const QSize& size);

    // Соединение линий
    void startMergeLinesMode();
    void cancelMergeLinesMode();
    bool handleMergeLinesClick(const QPointF& scenePos); // true если клик обработан
    bool performMergeLines(qgraph::Line* lineA, int indexA, qgraph::Line* lineB, int indexB);

    // Разрыв линии
    bool performSplitLineByEdge(qgraph::Line*, const QPointF& scenePos);

    // Нормализация нумерации
    static double signedArea2(const QVector<QPointF>& points);
    static int topLeftIndex(const QVector<QPointF>& points); // Нумерация по часовой
    static bool isBetterTopLeft(const QPointF& firstPoint, const QPointF& secondPoint);

    // Режим в statusBar
    void updateModeLabel();

    // Перемещение в режиме рисования
    void handlePolylineLmbClick(const QPointF& scenePos, Qt::KeyboardModifiers, GraphicsView*);
    void handleLineLmbClick(const QPointF& scenePos, Qt::KeyboardModifiers mods, GraphicsView* graphView);

    // Кнопки в списке фигур
    void moveCurrentShapeInList(int direction);
    void updateShapeListButtons();
    void refreshShapeListOrderRole();

    // Сохранение номера фигуры
    int nextShapeNumberForScene(QGraphicsScene* scene) const;
    int ensureShapeNumber(QGraphicsItem* item) const;
    QList<QGraphicsItem*> orderedShapeItemsForSave(Document::Ptr doc) const;

private:
    Ui::MainWindow* ui;
    static QUuidEx _applId;

    QLabel* _imageSizeLabel = {nullptr}; // Размер изображения в statusBar

    QString _currentFolderPath; // Переменную для хранения пути
    QString _lastUsedFolder; // Последняя открытая папка
    bool _openLastFolderOnFirstShow = {true};

    QPoint _lastMousePos; // Последняя позиция мыши

    QGraphicsItem* _draggingItem = {nullptr}; // Указатель на перетаскиваемый объект

    bool _drawingCircle    = {false}; // Флаг, рисуется ли круг
    bool _drawingLine      = {false}; // Флаг, рисуется ли линия
    bool _drawingPolyline  = {false}; // Флаг, рисуется ли полилиния
    bool _drawingRectangle = {false}; // Флаг, рисуется ли прямоугольник
    bool _drawingPoint     = {false};
    bool _isInDrawingMode  = {false}; // Флаг, указывающий что мы в режиме рисования новой фигуры
    bool _isDraggingImage  = {false}; // Флаг для перетаскивания изображения
    bool _isAllMoved       = {false}; // Флаг для перетаскивания изображения вместе с разметкой

    enum class PendingDrawTool {None, Polyline, Line};
    PendingDrawTool _pendingDrawTool = {PendingDrawTool::None};
    QPoint _pendingDrawPressViewPos;
    QPointF _pendingDrawPressScenePos;

    QPointF _startPoint;

    QGraphicsRectItem* _currRectangle = {nullptr};
    bool _isDrawingRectangle = {false};

    QGraphicsEllipseItem* _currCircle = {nullptr};
    bool _isDrawingCircle = {false};
    // Временный крестик центра при рисовании круга
    QGraphicsLineItem* _currCircleCrossV = {nullptr};
    QGraphicsLineItem* _currCircleCrossH = {nullptr};

    bool _isDrawingPolyline = {false}; // Флаг для состояния рисования
    bool _isDrawingPoint = {false};
    bool _isDrawingLine = {false};

    // Линейка
    QGraphicsLineItem* _rulerLine = {nullptr}; // Временная линия измерения
    QGraphicsTextItem* _rulerText = {nullptr}; // Подпись с расстоянием
    bool _drawingRuler = {false}; // Выбран ли инструмент "Линейка"
    bool _isDrawingRuler = {false}; // Тянем вторую точку
    QPointF _rulerStartPoint; // Первая точка измерения


    qgraph::Polyline* _polyline = {nullptr};
    qgraph::Line* _line = {nullptr};
    qgraph::Point* _currPoint = {nullptr};

    QMap<QString /*file path*/, Document::Ptr> _documentsMap;

    // Текущее изображение
    QString _currentImagePath;
    // Буфер для копирования фигур между сценами
    QString _shapesClipboard;

    QMap<QString /*class name*/, QColor> _projectClassColors;

    // Временные данные для рисования
    QGraphicsRectItem* _tempRectItem = {nullptr};
    QGraphicsEllipseItem* _tempCircleItem = {nullptr};
    qgraph::Polyline* _tempPolyline = {nullptr};

    qreal _m_zoom = 1.0;
    static constexpr qreal _kZoomStep = {1.10};
    static constexpr qreal _kMinZoom  = {0.10};
    static constexpr qreal _kMaxZoom  = {100};

    QList<int> _savedSplitterSizes; // Хранит нормальные размеры сплиттера
    bool _isRightSplitterCollapsed = {false};

    // Механика призрачной ручки
    QGraphicsRectItem* _ghostHandle = {nullptr}; // Рисуемая сверху копия
    qgraph::DragCircle* _ghostTarget = {nullptr}; // Реальная ручка
    bool _ghostActive = {false};
    bool _ghostHover  = {false};
    QPointF _ghostGrabOffset; // Смещение точки хвата
    qreal _ghostPickRadius = {6.0}; // Радиус поиска ручки под курсором
    qreal _edgePickRadius  = {8.0}; // Радиус захвата ребер
    qreal _drawHandleCommitRadius = {0.01}; // Точное попадание в узел именно во время рисования line/polyline

    // Состояние перетаскивания
    bool _m_isDraggingHandle = {false};
    QPointer<qgraph::DragCircle> _m_dragHandle; // Какую ручку тащим
    QPointF _m_pressLocalOffset;                // Смещение от центра ручки при захвате

    // Состояние левой кнопки мыши над графическим видом
    bool _leftMouseButtonDown = {false};

    bool _loadingNow = {false};
    bool _syncingSelection = {false};
    bool _handleDragging = {false}; // Флаг для отслеживания перетаскивания ручек

    qgraph::DragCircle* _currentHoveredHandle = {nullptr};
    qgraph::DragCircle* _lastHoverHandle = {nullptr}; // Кто сейчас в hover-стиле

    struct VisualStyle
    {
        qreal lineWidth = 2.0;
        qreal handleSize = 10.0;
        qreal numberFontPt = 10.0;
        bool showNumbers = true;
        bool showSelectionFrame = true;
        bool fillShapeWhenSelected = true;
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
        QColor rulerColor = QColor(200, 200, 200);
        int pointOutlineWidth = 1;
        int pointSize = 6;

    };
    VisualStyle _vstyle; // Глобально для всех фигур

    QUndoGroup* _undoGroup = {nullptr};
    QUndoView* _undoView = {nullptr}; // Ссылка на вид из .ui
    QStringList _projectClasses;    // Единый список классов проекта
    ProjectSettings* _projPropsDialog = {nullptr};

    // Перемещение фигур
    QGraphicsItem* _movingItem = {nullptr};
    bool           _moveHadChanges = {false};
    bool           _moveInProgress = {false};

    QPointF        _moveGrabOffsetScene; // Смещение узла относительно item->scenePos()
    QGraphicsItem::GraphicsItemFlags  _moveSavedFlags{}; // Чтобы вернуть флаги

    // Групповое перемещение фигур
    QVector<QGraphicsItem*> _movingItems; // Все фигуры, которые двигаем вместе
    QVector<QPointF> _moveGroupInitialPos; // Начальные позиции при захвате
    bool _moveIsGroup = {false}; // true, если тянем сразу несколько фигур
    QPointF _movePressScenePos; // Позиция курсора в сцене в момент захвата

    QGraphicsItem* _handleEditedItem = {nullptr}; // Владелец перетаскиваемой ручки

    // Минимальное состояние редактирования ручки
    int     _handleEditedIndex = {-1};
    QPointF _handlePointBefore;
    QRectF  _handleRectBefore;
    qreal   _handleCircleRadiusBefore = {0};

    bool    _handleDragHadChanges = {false};

    // Соединение линий
    bool _mergeLinesMode = {false};
    qgraph::Line* _mergeLineA = {nullptr};
    int _mergeLineAEndIdx = -1; // Первая или последняя

    bool _shiftImageDragging = {false};

    // Продолжение рисования
    bool _resumeEditing = {false};
    qulonglong _resumeUid = {0};

    // Зум прямоугольной области Ctrl + ЛКМ
    bool _zoomRectActive = {false};
    QPoint _zoomRectOrigin;
    QPointer<QRubberBand> _zoomRubberBand;
    QPointer<GraphicsView> _zoomRectView;
    bool _zoomRectWasInteractive = {true};

    QHash<qulonglong, qreal> _temporaryRaisedZValues; // Для хранения z-уровней фигур

    // Стабильный ключ в QGraphicsItem::data(...)
    static constexpr int _roleUid = 0x1337ABCD;
    static constexpr int _roleListOrder = 0x1337ABCE;
    static constexpr int _roleShapeNumber = 0x1337ABCF;

    // Счетчик уникальных id на время жизни документа
    mutable qulonglong _uidCounter = {1};

    QLabel* _modeLabel = {nullptr}; // Режим в statusBar

    // Позволяем стороннему классу видеть все
    //friend class GraphicsView;
};

//Q_DECLARE_METATYPE(Document::Ptr)

