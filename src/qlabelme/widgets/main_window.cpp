#include "main_window.h"
#include "ui_main_window.h"
#include "load_geometry.h"
#include "undo_stack.h"

#include "shared/defmac.h"
#include "shared/break_point.h"
#include "shared/steady_timer.h"
#include "shared/logger/logger.h"
#include "shared/logger/format.h"
#include "shared/config/appl_conf.h"
#include "shared/qt/logger_operators.h"

// #include "pproto/commands/base.h"
// #include "pproto/commands/pool.h"
// #include "pproto/logger_operators.h"

#include "qgraphics2/circle.h"
#include "qgraphics2/rectangle.h"
#include "qgraphics2/polyline.h"
#include "qgraphics2/square.h"
#include "qgraphics2/point.h"
#include "qgraphics2/line.h"

//#include "qgraphics/functions.h"
//#include "qutils/message_box.h"

#include "select_class.h"
#include "settings.h"
#include "user_guide.h"
#include "about_program.h"
#include "message_box.h"
#include "unsaved_changes.h"

#include <QApplication>
#include <QMessageBox>
#include <QFileDialog>
#include <QScrollBar>

using namespace std;
using namespace qgraph;

#define log_error_m   alog::logger().error   (alog_line_location, "MainWin")
#define log_warn_m    alog::logger().warn    (alog_line_location, "MainWin")
#define log_info_m    alog::logger().info    (alog_line_location, "MainWin")
#define log_verbose_m alog::logger().verbose (alog_line_location, "MainWin")
#define log_debug_m   alog::logger().debug   (alog_line_location, "MainWin")
#define log_debug2_m  alog::logger().debug2  (alog_line_location, "MainWin")

QUuidEx MainWindow::_applId;

namespace
{
    // Строковый идентификатор формата данных
    const char* kShapesMimeType = "application/x-qlabelme-shapes";
}

class OneSpinDialog : public QDialog {
public:
    explicit OneSpinDialog(const QString& title,
                           const QString& label,
                           double minValue, double maxValue, double step,
                           double value,
                           QWidget* parent=nullptr)
        : QDialog(parent)
    {
        setWindowTitle(title);
        QDoubleSpinBox* spin = new QDoubleSpinBox; spin->setRange(minValue, maxValue);
                    spin->setDecimals(1); spin->setSingleStep(step); spin->setValue(value);
        QFormLayout* form = new QFormLayout; form->addRow(label, spin);
        QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
        QVBoxLayout* dialogLayout = new QVBoxLayout(this); dialogLayout->addLayout(form);
                            dialogLayout->addWidget(buttonBox);
        _spin = spin;
    }
    double value() const { return _spin->value(); }
private:
    QDoubleSpinBox* _spin = nullptr;
};

// Идем вверх по родителям, ищем перемещаемого владельца
static QGraphicsItem* findMovableAncestor(QGraphicsItem* item)
{
    for (QGraphicsItem* currentItem = item; currentItem; currentItem = currentItem->parentItem())
    {
        if (currentItem->flags().testFlag(QGraphicsItem::ItemIsMovable))
            return currentItem;
    }
    return item;
}

MainWindow::MainWindow(QWidget* parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
    //_scene(new QGraphicsScene(this))
    //_socket(new tcp::Socket)
{
    ui->setupUi(this);

    ui->graphView->setViewportUpdateMode(QGraphicsView::MinimalViewportUpdate);
    ui->graphView->setCacheMode(QGraphicsView::CacheBackground);

    //ui->menuBar->raise();

    // toolbar в правую панель над "Фигуры/Координаты/Стек действий"
    // Оба тулбара в layoutWidget1
    ui->toolBar->setParent(ui->layoutWidget1);
    ui->toolBar->setAllowedAreas(Qt::NoToolBarArea);
    ui->toolBar->setFloatable(false);
    ui->toolBar->setMovable(false);

    ui->toolBar_2->setParent(ui->layoutWidget1);
    ui->toolBar_2->setAllowedAreas(Qt::NoToolBarArea);
    ui->toolBar_2->setFloatable(false);
    ui->toolBar_2->setMovable(false);

    // Создаем контейнер для тулбаров и кладем их рядом
    QWidget* toolBarsBlock = new QWidget(ui->layoutWidget1);

    QVBoxLayout* vtb = new QVBoxLayout(toolBarsBlock);
    vtb->setContentsMargins(0, 0, 0, 0);
    vtb->setSpacing(0); // Расстояние между тулбарами

    // Строка для тулбаров
    QWidget* toolBarsRow = new QWidget(toolBarsBlock);
    QHBoxLayout* htb = new QHBoxLayout(toolBarsRow);
    htb->setContentsMargins(0, 0, 0, 0);
    htb->setSpacing(8);

    ui->toolBar->setStyleSheet("QToolBar{border:none; background:transparent;} QToolButton{padding:2px;}");
    ui->toolBar_2->setStyleSheet("QToolBar{border:none; background:transparent;} QToolButton{padding:2px;}");

    // Чтобы первый тулбар не растягивался, а второй занимал остаток
    ui->toolBar->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    ui->toolBar_2->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // Вертикальный разделитель между тулбарами
    QFrame* vSep = new QFrame(toolBarsRow);
    vSep->setFixedWidth(1);
    vSep->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    vSep->setStyleSheet("background-color: #505050;");

    // Выравниваем по центру по вертикали
    htb->addWidget(ui->toolBar, 0, Qt::AlignVCenter);
    htb->addWidget(vSep);
    htb->addWidget(ui->toolBar_2, 1, Qt::AlignVCenter);

    htb->setStretch(0, 0); // toolBar_2 не растягиваем
    htb->setStretch(1, 0);
    htb->setStretch(2, 1);

    ui->toolBar->setMinimumWidth(ui->toolBar->sizeHint().width());

    // Нижняя общая линия под тулбарами
    QFrame* hSep = new QFrame(toolBarsBlock);
    hSep->setFrameShape(QFrame::HLine);
    hSep->setFrameShadow(QFrame::Plain);
    hSep->setFixedHeight(1);
    hSep->setStyleSheet("background-color:#3a3a3a;");

    vtb->addWidget(toolBarsRow);
    vtb->addWidget(hSep);

    // Убираем старые виджеты из сетки и вставляем блок тулбаров в row 0
    ui->gridLayout->removeWidget(ui->figuresWidget);
    ui->gridLayout->removeWidget(ui->label);
    ui->gridLayout->removeWidget(ui->label_4);
    ui->gridLayout->removeWidget(ui->polygonList);
    ui->gridLayout->removeWidget(ui->coordinateList);
    ui->gridLayout->removeWidget(ui->undoView);

    // Добавляем контейнер toolBarsBlock
    ui->gridLayout->addWidget(toolBarsBlock, 0, 0, 1, 3);

    // Возвращаем заголовки и списки ниже
    ui->gridLayout->addWidget(ui->figuresWidget, 1, 0);
    ui->gridLayout->addWidget(ui->label,   1, 1);
    ui->gridLayout->addWidget(ui->label_4, 1, 2);

    ui->gridLayout->addWidget(ui->polygonList,    2, 0);
    ui->gridLayout->addWidget(ui->coordinateList, 2, 1);
    ui->gridLayout->addWidget(ui->undoView,       2, 2);

    // Растяжение: тулбары/заголовки не растягиваются, низ растягивается
    ui->gridLayout->setRowStretch(0, 0);
    ui->gridLayout->setRowStretch(1, 0);
    ui->gridLayout->setRowStretch(2, 1);


    ui->graphView->setMouseTracking(true);
    ui->graphView->viewport()->setMouseTracking(true);

    ui->graphView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->graphView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    ui->actScrollBars->setCheckable(true);
    ui->actScrollBars->setChecked(false); // По умолчанию скрыто

    ui->graphView->setResizeAnchor(QGraphicsView::AnchorViewCenter);
    ui->graphView->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    ui->graphView->setAlignment(Qt::AlignCenter);


    // Сделать подсветку одинаковой яркой, даже когда список без фокуса
    void (*fixSelectionPalette)(QAbstractItemView*) = [](QAbstractItemView* itemView)
    {
        QPalette palette = itemView->palette();
        const QColor activeHighlight = palette.color(QPalette::Active, QPalette::Highlight);
        const QColor activeHighlightedText = palette.color(QPalette::Active, QPalette::HighlightedText);

        palette.setColor(QPalette::Inactive, QPalette::Highlight, activeHighlight);
        palette.setColor(QPalette::Inactive, QPalette::HighlightedText, activeHighlightedText);
        palette.setColor(QPalette::Disabled, QPalette::Highlight, activeHighlight);
        palette.setColor(QPalette::Disabled, QPalette::HighlightedText, activeHighlightedText);

        itemView->setPalette(palette);
        if (itemView->viewport())
            itemView->viewport()->setPalette(palette);
    };

    fixSelectionPalette(ui->polygonList);
    fixSelectionPalette(ui->fileList);

    qRegisterMetaType<QGraphicsItem*>("QGraphicsItem*");

    ui->graphView->viewport()->installEventFilter(this);
    loadVisualStyle();

    bool ultraHD = false;
    QList<QScreen*> screens = QGuiApplication::screens();
    if (!screens.isEmpty())
        ultraHD = (screens[0]->geometry().width() >= 2560);

    // if (ultraHD)
    //     ui->toolBar->setIconSize({48, 48});
    // else
    //     ui->toolBar->setIconSize({32, 32});
#ifdef Q_OS_WINDOWS
    ui->toolBar->setIconSize({32, 32});
#else
    if (ultraHD)
        ui->toolBar->setIconSize({32, 32});
    else
        ui->toolBar->setIconSize({24, 24});
#endif

    _windowTitle = windowTitle();

    ui->graphView->init(this);
    //setWindowTitle(windowTitle() + QString(" (%1)").arg(VERSION_PROJECT));

//    enableButtons(false);
//    disableAdminMode();

    //ui->graphView->setScene(_scene);
    // connect(_scene, &QGraphicsScene::changed,
    //         this,  &MainWindow::onSceneChanged);

    // _videoRect = new qgraph::VideoRect(_scene);

    //_labelConnectStatus = new QLabel(u8"Нет подключения", this);
    //ui->statusBar->addWidget(_labelConnectStatus);

    ui->graphView->viewport()->installEventFilter(this);
    ui->graphView->setMouseTracking(true);    


    QString buildDate =
            QLocale(QLocale::English)
            .toDate(QString(__DATE__), "MMM d yyyy")
            .toString("dd.MM.yyyy");

    QString vers = u8"Версия: %1 | Дата: %2 | git: %3";
    vers = vers.arg(VERSION_PROJECT)
               .arg(buildDate)
               .arg(GIT_REVISION);

    ui->statusBar->addPermanentWidget(new QLabel(vers, this));

    _imageSizeLabel = new QLabel(this);
    _imageSizeLabel->setText(u8"Размер изображения: —");
    _imageSizeLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    statusBar()->addWidget(_imageSizeLabel, 0);

    // Режим в statusBar
    _modeLabel = new QLabel(this);
    _modeLabel->setText(u8"Режим: просмотр");
    _modeLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    _modeLabel->setObjectName("modeLabel");
    _modeLabel->setMinimumWidth(220);

    _modeLabel->setStyleSheet(
        "QLabel#modeLabel {"
        "  padding: 2px 8px;"
        "  border: 1px solid rgba(0,0,0,60);"
        "  border-radius: 6px;"
        "  background: rgba(0,0,0,25);"
        "}"
    );
    statusBar()->addWidget(_modeLabel, 0);
    updateModeLabel();

    QString fileName = "/home/marie/фон.png";
    QPixmap pix(fileName);
    //_videoRect->setPixmap(pix);

    // Масштабируем фон под размер окна
    pix = pix.scaled(ui->graphView->size(), Qt::KeepAspectRatioByExpanding);
    // Устанавливаем изображение фоном для сцены
    ui->graphView->setBackgroundBrush(pix);

    // Инициализация с пустой папкой
    _currentFolderPath = "";


    // TODO все коннекты исправить на chk_connect_a
    chk_connect_a(ui->fileList, &QListWidget::currentItemChanged,
                  this, &MainWindow::fileList_ItemChanged);

    chk_connect_a(ui->polygonList, &QListView::clicked,
            this, &MainWindow::onPolygonListItemClicked);

    chk_connect_a(ui->polygonList, &QListView::doubleClicked,
            this, &MainWindow::onPolygonListItemDoubleClicked);

    chk_connect_a(ui->polygonList, &QWidget::customContextMenuRequested,
            this, &MainWindow::showPolygonListContextMenu);


    QShortcut* shNext = new QShortcut(QKeySequence(Qt::Key_D), this);
    shNext->setContext(Qt::ApplicationShortcut);
    connect(shNext, &QShortcut::activated, this, &MainWindow::nextImage);

    QShortcut* shPrev = new QShortcut(QKeySequence(Qt::Key_A), this);
    shPrev->setContext(Qt::ApplicationShortcut);
    connect(shPrev, &QShortcut::activated, this, &MainWindow::prevImage);

    QShortcut* shSelectAll = new QShortcut(QKeySequence::SelectAll, this);
    shSelectAll->setContext(Qt::ApplicationShortcut);
    connect(shSelectAll, &QShortcut::activated, this, &MainWindow::selectAllShapes);

    ui->polygonList->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->polygonList->setSelectionMode(QAbstractItemView::ExtendedSelection);

    ui->btnMoveShapeUp->setToolTip(QString::fromUtf8("Переместить фигуру выше"));
    ui->btnMoveShapeDown->setToolTip(QString::fromUtf8("Переместить фигуру ниже"));
    ui->btnDelete->setToolTip(QString::fromUtf8("Удалить выбранную фигуру"));

    ui->btnMoveShapeUp->setAutoRaise(true);
    ui->btnMoveShapeDown->setAutoRaise(true);
    ui->btnDelete->setAutoRaise(true);

    connect(ui->btnMoveShapeUp, &QToolButton::clicked,
            this, [this]()
    {
        moveCurrentShapeInList(-1);
    });

    connect(ui->btnMoveShapeDown, &QToolButton::clicked,
            this, [this]()
    {
        moveCurrentShapeInList(1);
    });

    connect(ui->btnDelete, &QToolButton::clicked,
            this, &MainWindow::on_actDelete_triggered);


    updateShapeListButtons();

    ui->graphView->viewport()->setMouseTracking(true);
    ui->graphView->setMouseTracking(true);

    //_scene->installEventFilter(this);

//    chk_connect_q(_socket.get(), &tcp::Socket::message,
//                  this, &MainWindow::message)

//    chk_connect_q(_socket.get(), &tcp::Socket::connected,
//                  this, &MainWindow::socketConnected)

//    chk_connect_q(_socket.get(), &tcp::Socket::disconnected,
//                  this, &MainWindow::socketDisconnected)


/*
    #define FUNC_REGISTRATION(COMMAND) \
        _funcInvoker.registration(command:: COMMAND, &MainWindow::command_##COMMAND, this);

    FUNC_REGISTRATION(DevicesInfo)

    #undef FUNC_REGISTRATION
*/

    ui->splitter->setSizes({INT_MAX, INT_MAX});
    // Сохраняем указатель на правую панель
    _rightPanel = ui->splitter->widget(1);
    // Сохраняем начальные размеры сплиттера
    _savedSplitterSizes = ui->splitter->sizes();
    applyLabelFontToUi();

    // сцена должна отдавать нам события
    //_scene->installEventFilter(this);

    // нужен свободный трекинг мыши
    ui->graphView->viewport()->setMouseTracking(true);

    _lastHoverHandle = nullptr;

    int cmPolyline = static_cast<int>(Settings::PolylineCloseMode::DoubleClick);
    config::base().getValue("polyline.close_mode", cmPolyline);
    _polylineCloseMode = static_cast<Settings::PolylineCloseMode>(cmPolyline);
    applyClosePolyline();

    int cmLine = static_cast<int>(Settings::LineFinishMode::DoubleClick);
    config::base().getValue("line.finish_mode", cmLine);
    _lineFinishMode = static_cast<Settings::LineFinishMode>(cmLine);
    applyFinishLine();

    //ensureGhostAllocated();

    //config::base().getValue("view.keep_image_scale", init.keepImageScale);

    // TODO Группа стеков действий для разных документов
    _undoGroup = new QUndoGroup(this);

    // Включаемость по состоянию группы
    connect(_undoGroup, &QUndoGroup::canUndoChanged, ui->actUndo, &QAction::setEnabled);
    connect(_undoGroup, &QUndoGroup::canRedoChanged, ui->actRedo, &QAction::setEnabled);

    // Начальное состояние
    ui->actUndo->setEnabled(_undoGroup->canUndo());
    ui->actRedo->setEnabled(_undoGroup->canRedo());

    ui->actUndo->setShortcuts(QKeySequence::keyBindings(QKeySequence::Undo));
    ui->actRedo->setShortcuts(QKeySequence::keyBindings(QKeySequence::Redo));

    ui->actUndo->setShortcutContext(Qt::ApplicationShortcut);
    ui->actRedo->setShortcutContext(Qt::ApplicationShortcut);

    ui->toolBar->setIconSize(QSize(32, 32));

    _undoView = ui->undoView;
    _undoView->setEmptyLabel(u8"<пусто>");

    // Классы в настройках
    _projPropsDialog = new ProjectSettings(this);

    connect(_projPropsDialog, &ProjectSettings::classesApplied, this,
        [this](const QStringList& classes){
            _projectClasses = classes;

            _projectClassColors = _projPropsDialog->projectClassColors();

            if (!saveProjectClasses(_projectClasses, _projectClassColors))
            {
                messageBox(
                    this,
                    QMessageBox::Warning,
                    u8"Не удалось сохранить classes.yaml"
                );
            }
        });

    connect(_projPropsDialog, &ProjectSettings::classColorsApplied, this,
    [this](const QMap<QString, QColor>& colors)
    {
        _projectClassColors = colors;

        QMap<QString, Document::Ptr>::const_iterator it = _documentsMap.constBegin();

        while (it != _documentsMap.constEnd())
        {
            Document::Ptr doc = it.value();

            if (doc && doc->scene)
            {
                const QList<QGraphicsItem*> items = doc->scene->items();

                for (QGraphicsItem* item : items)
                {
                    if (!item)
                        continue;

                    const QString cls = item->data(0).toString();

                    if (!cls.isEmpty())
                        applyClassColorToItem(item, cls);
                }

                doc->scene->update();
            }

            ++it;
        }
    });

    connect(ui->actUserGuide, &QAction::triggered, this, [this]()
    {
        UserGuide* guide = new UserGuide(this);
        guide->show();
    });
    connect(ui->actAbout, &QAction::triggered, this, [this]()
    {
        AboutProgram* about = new AboutProgram(this);
        about->show();
    });

    // Чтобы Alt работал независимо от фокуса
    qApp->installEventFilter(this);

    // Восстановить видимость меню
    bool keepMenuVis = false;
    config::base().getValue("ui.keep_menu_visibility", keepMenuVis);

    bool mbVisible = true; // по умолчанию при запуске меню видно
    if (keepMenuVis)
        config::base().getValue("ui.menu_visible", mbVisible);

    ui->menuBar->setVisible(mbVisible);

    // Чтобы шорткаты работали даже при скрытом menuBar
    const QList<QAction*> acts = findChildren<QAction*>();
    for (QAction* act : acts)
    {
        if (!act)
            continue;

        if (act->menu() != nullptr)
            continue;

        if (act->shortcut().isEmpty())
            continue;

        act->setShortcutContext(Qt::WindowShortcut);
        this->addAction(act);
    }
}

MainWindow::~MainWindow()
{
    _loadingNow = true;

    if (ui)
    {
        if (ui->graphView)
            ui->graphView->setScene(nullptr);

        if (ui->polygonList)
            ui->polygonList->setModel(nullptr);

        if (ui->coordinateList)
            ui->coordinateList->clear();
    }

    if (_undoView)
        _undoView->setStack(nullptr);

    if (_undoGroup)
        _undoGroup->setActiveStack(nullptr);

    for (QMap<QString, Document::Ptr>::iterator it = _documentsMap.begin();
         it != _documentsMap.end(); ++it)
    {
        Document::Ptr doc = it.value();

        if (!doc)
            continue;

        if (doc->_undoStack)
        {
            QObject::disconnect(doc->_undoStack.get(), nullptr, this, nullptr);

            if (_undoGroup)
                _undoGroup->removeStack(doc->_undoStack.get());
        }

        QObject::disconnect(&doc->undoStack2, nullptr, this, nullptr);

        QGraphicsScene* scene = doc->scene;
        if (scene)
            QObject::disconnect(scene, nullptr, this, nullptr);
    }

    if (ui && ui->fileList)
        ui->fileList->clear();

    _documentsMap.clear();

    delete ui;
}

bool MainWindow::init()
{
    if (config::base().getValue("application.id", _applId, false))
    {
        log_verbose_m << "Read ApplId: " << _applId;
    }
    else
    {
        _applId = QUuidEx::createUuid();
        config::base().setValue("application.id", _applId);
        config::base().saveFile();
        log_verbose_m << "Generated new ApplId: " << _applId;
    }

    loadLastUsedFolder(); // Загружаем последнюю использованную папку
    loadGeometry();
    loadVisualStyle();    // Загружаем визуальный стиль
    return true;
}

void MainWindow::deinit()
{
}

void MainWindow::graphicsView_mousePressEvent(QMouseEvent* mouseEvent, GraphicsView* graphView)
{
    const std::function<bool()> isAnyDrawingNow = [&]() -> bool
    {
        return _isInDrawingMode
            || _isDrawingPolyline || _isDrawingLine
            || _isDrawingRectangle || _isDrawingCircle || _isDrawingPoint
            || _isDrawingRuler
            || _drawingPolyline  || _drawingLine  || _drawingRectangle
            || _drawingCircle  || _drawingPoint
            || _drawingRuler;
    };
    Document::Ptr doc = currentDocument();
    if (!doc || !doc->scene)
        return;
    QGraphicsScene* scene = doc->scene;
    qgraph::VideoRect* videoRect = doc->videoRect;

    // Если мы в режиме рисования, то не меняем режим при нажатии на _videoRect
    if (isAnyDrawingNow())
    {
        _isDraggingImage = false;
        _isAllMoved = false;
        _shiftImageDragging = false;
        updateModeLabel();
    }

    if (mouseEvent->button() == Qt::LeftButton)
        _leftMouseButtonDown = true;

    // Alt + ЛКМ / Alt + Shift + ЛКМ для редактирования узлов
    if (mouseEvent->button() == Qt::LeftButton && graphView)
    {
        //const Qt::KeyboardModifiers mods = mouseEvent->modifiers();
        const Qt::KeyboardModifiers mods =
                mouseEvent->modifiers() | QApplication::keyboardModifiers();
        const QPointF scenePos = graphView->mapToScene(mouseEvent->pos());

        // Alt + ЛКМ добавить узел на ребро
        if ((mods & Qt::AltModifier) && !(mods & Qt::ShiftModifier))
        {
            QGraphicsItem* item = pickItemByEdgeAt(graphView, mouseEvent->pos());

            if (qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(item))
            {
                int pointIndex = -1;
                QPointF point;

                if (polyline->findInsertPointAt(scenePos, &pointIndex, &point))
                {
                    QUndoStack* stack = activeUndoStack();

                    if (!stack)
                        return;

                    if (polyline->insertPointAtIndex(pointIndex, point))
                    {
                        undo::NodeEdit::Data data;
                        data.type = undo::NodeEdit::Type::InsertPolylineNode;
                        data.pointIndex = pointIndex;
                        data.point = point;

                        stack->push(new undo::NodeEdit(doc.get(),
                                                       polyline,
                                                       data,
                                                       u8"Добавление узла"));

                        if (!doc->isModified)
                        {
                            doc->isModified = true;
                            updateFileListDisplay(doc->filePath);
                        }

                        mouseEvent->accept();
                        return;
                    }
                }
            }
            if (qgraph::Line* line = dynamic_cast<qgraph::Line*>(item))
            {
                int pointIndex = -1;
                QPointF point;

                if (line->findInsertPointAt(scenePos, &pointIndex, &point))
                {
                    QUndoStack* stack = activeUndoStack();

                    if (!stack)
                        return;

                    if (line->insertPointAtIndex(pointIndex, point))
                    {
                        undo::NodeEdit::Data data;
                        data.type = undo::NodeEdit::Type::InsertLineNode;
                        data.pointIndex = pointIndex;
                        data.point = point;

                        stack->push(new undo::NodeEdit(doc.get(),
                                                       line,
                                                       data,
                                                       u8"Добавление узла"));

                        if (!doc->isModified)
                        {
                            doc->isModified = true;
                            updateFileListDisplay(doc->filePath);
                        }

                        mouseEvent->accept();
                        return;
                    }
                }
            }
        }
    }

    // Зум прямоугольной области Ctrl + ЛКМ
    if (graphView &&
        mouseEvent->button() == Qt::LeftButton &&
        (mouseEvent->modifiers() & Qt::ControlModifier) &&
        !(mouseEvent->modifiers() & Qt::ShiftModifier) &&
        !_drawingPolyline &&
        !_drawingLine &&
        !_drawingRectangle &&
        !_drawingCircle &&
        !_drawingPoint &&
        !_drawingRuler)
    {
        _zoomRectActive = true;
        updateModeLabel();

        // Чтобы фигуры/сцена не начали двигаться во время возможного зума
        _leftMouseButtonDown = false;

        _zoomRectView = graphView;
        _zoomRectWasInteractive = graphView->isInteractive();
        _zoomRectOrigin = mouseEvent->pos();

        if (_zoomRubberBand)
            _zoomRubberBand->hide();

        mouseEvent->accept();
        return;
    }
    // Было написано для создания иконки
    if ((mouseEvent->modifiers() & Qt::ControlModifier) &&
        (mouseEvent->button() == Qt::RightButton))
    {
        if (!graphView || !graphView->scene())
            return;

        const QPointF scenePos = graphView->mapToScene(mouseEvent->pos());

        // Находим полилинию под курсором (самую верхнюю)
        qgraph::Polyline* poly = nullptr;
        const QList<QGraphicsItem*> itemsAtPos = graphView->scene()->items(scenePos);
        for (QGraphicsItem* item : itemsAtPos)
        {
            if (qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(item))
            {
                poly = polyline;
                break;
            }
        }
        if (!poly) return;

        // Ищем ближайшую точку
        DragCircle* nearest = nullptr;
        qreal bestDist2 = 10.0 * 10.0; // радиус 10 px
        for (DragCircle* circles : poly->circles())
        {
            const QPointF polyline = circles->scenePos();
            const qreal dx = polyline.x() - scenePos.x();
            const qreal dy = polyline.y() - scenePos.y();
            const qreal d2 = dx*dx + dy*dy;
            if (d2 < bestDist2)
            {
                bestDist2 = d2;
                nearest = circles;
            }
        }
        if (nearest)
        {
            nearest->setUserHidden(!nearest->isUserHidden());
        }

        return;
    }
    if (mouseEvent->button() == Qt::LeftButton && _drawingRuler)
    {
        if (!_isDrawingRuler)
        {
            Document::Ptr doc = currentDocument();

            if (!doc || !doc->scene || !doc->videoRect)
                return;

            QGraphicsScene* scene = doc->scene;
            qgraph::VideoRect* videoRect = doc->videoRect;

            _rulerStartPoint = graphView->mapToScene(mouseEvent->pos());
            _isDrawingRuler = true;
            updateModeLabel();

            if (_rulerLine)
            {
                scene->removeItem(_rulerLine);
                delete _rulerLine;
                _rulerLine = nullptr;
            }
            if (_rulerText)
            {
                scene->removeItem(_rulerText);
                delete _rulerText;
                _rulerText = nullptr;
            }

            QPen pen(_vstyle.rulerColor);
            pen.setWidthF(2.0);
            pen.setStyle(Qt::DotLine);
            pen.setCapStyle(Qt::RoundCap);
            pen.setJoinStyle(Qt::RoundJoin);
            pen.setCosmetic(true);

            _rulerLine = scene->addLine(QLineF(_rulerStartPoint, _rulerStartPoint), pen);
            _rulerLine->setZValue(1000);

            _rulerText = scene->addText(QStringLiteral("0"));
            _rulerText->setDefaultTextColor(_vstyle.rulerColor);
            // Масштабируем размер текста от размера изображения
            if (videoRect)
            {
                const QSize imgSize = videoRect->pixmap().size();
                const int maxSide = std::max(imgSize.width(), imgSize.height());

                QFont f = _rulerText->font();
                f.setPointSizeF(std::max(6.0, maxSide / 200.0));
                _rulerText->setFont(f);
            }
            _rulerText->setZValue(1001);
            _rulerText->setPos(_rulerStartPoint);
        }

        _drawingRuler = false;
        mouseEvent->accept();
        return;
    }
    if (mouseEvent->button() == Qt::LeftButton && !isAnyDrawingNow())
    {
        QGraphicsItem* clickedItem = graphView->itemAt(mouseEvent->pos());
        if (!clickedItem)
            clickedItem = pickItemByEdgeAt(graphView, mouseEvent->pos());

        QGraphicsItem* selectionItem = clickedItem;

        if (qgraph::DragCircle* handle = dynamic_cast<qgraph::DragCircle*>(selectionItem))
            selectionItem = handle->parentItem();

        if (selectionItem && selectionItem != videoRect)
            selectionItem = findMovableAncestor(selectionItem);

        // Выделение через сцену, включая мультивыделение по Ctrl
        if (selectionItem && selectionItem != videoRect)
        {
            const bool ctrlPressed = mouseEvent->modifiers() & Qt::ControlModifier;

            if (ctrlPressed)
            {
                selectionItem->setSelected(!selectionItem->isSelected());

                _movingItem = nullptr;
                _movingItems.clear();
                //_moveGroupBefore.clear();
                _moveGroupInitialPos.clear();
                _moveIsGroup = false;
                _moveHadChanges = false;
                _moveInProgress = false;

                _draggingItem = nullptr;
                _isDraggingImage = false;
                _shiftImageDragging = false;
                _isAllMoved = false;

                _lastMousePos = mouseEvent->pos();
                mouseEvent->accept();
                return;
            }
            else
            {
                const QList<QGraphicsItem*> selectedNow = scene->selectedItems();

                // Если кликнули по уже выделенной фигуре, и выделено несколько фигур,
                // то сохраняем групповое выделение для последующего перетаскивания
                if (!(selectedNow.size() > 1 && selectionItem->isSelected()))
                {
                    for (QGraphicsItem* item : selectedNow)
                    {
                        if (item && (item != selectionItem))
                            item->setSelected(false);
                    }
                    selectionItem->setSelected(true);
                }
            }
        }
        else if (!(mouseEvent->modifiers() & Qt::ControlModifier))
        {
            const QList<QGraphicsItem*> selectedNow = scene->selectedItems();
            for (QGraphicsItem* item : selectedNow)
            {
                if (item)
                    item->setSelected(false);
            }
        }

        // Сбрасываем состояния перемещения
        _movingItem = nullptr;
        _movingItems.clear();
        //_moveGroupBefore.clear();
        _moveGroupInitialPos.clear();
        _moveIsGroup = false;
        _moveHadChanges = false;
        _moveInProgress = false;

        _draggingItem = nullptr;
        _isDraggingImage = false;
        _shiftImageDragging = false;
        _isAllMoved = false;

        const bool drawingLineLike =
                _drawingLine || _drawingPolyline || _isDrawingLine || _isDrawingPolyline;

        if (drawingLineLike)
        {
            // Во время рисования line/polyline не захватываем фигуры
        }
        // Решили убрать на вермя, пока этот функционал не нужен
        // else if (mouseEvent->modifiers() & Qt::ShiftModifier)
        // {
        //     if (clickedItem == _videoRect && _videoRect)
        //     {
        //         _shiftImageBeforePos = _videoRect->pos();
        //         _shiftImageDragging = true;
        //         _lastMousePos = mouseEvent->pos();
        //         mouseEvent->accept();
        //         updateModeLabel();
        //         return;
        //     }

        //     if (clickedItem && !qgraphicsitem_cast<qgraph::DragCircle*>(clickedItem))
        //         _draggingItem = findMovableAncestor(clickedItem);

        //     _lastMousePos = mouseEvent->pos();
        //     mouseEvent->accept();
        //     updateModeLabel();
        //     return;
        // }
        else
        {
            const bool shiftPressed = mouseEvent->modifiers() & Qt::ShiftModifier;

            // Shift + ЛКМ по фигуре - перемещение фигуры
            if (shiftPressed)
            {
                if (selectionItem && selectionItem != videoRect)
                {
                    QGraphicsItem* movableItem = findMovableAncestor(selectionItem);

                    if (movableItem && (movableItem != videoRect)
                        && movableItem->flags().testFlag(QGraphicsItem::ItemIsMovable))
                    {
                        _movingItem = movableItem;
                        _moveHadChanges = false;
                        _moveInProgress = true;

                        const QPointF scenePos = graphView->mapToScene(mouseEvent->pos());
                        _moveGrabOffsetScene = scenePos - _movingItem->scenePos();
                        _moveSavedFlags = _movingItem->flags();

                        // Отключаем штатное Qt-перемещение, двигаем сами через _movingItem
                        _movingItem->setFlag(QGraphicsItem::ItemIsMovable, false);

                        _movingItems.clear();
                        _moveGroupInitialPos.clear();
                        _moveIsGroup = false;
                        _movePressScenePos = scenePos;

                        const QList<QGraphicsItem*> selectedItems = scene->selectedItems();
                        if (selectedItems.size() > 1 && movableItem->isSelected())
                        {
                            _moveIsGroup = true;

                            for (QGraphicsItem* selectedItem : selectedItems)
                            {
                                if (!selectedItem)
                                    continue;

                                QGraphicsItem* root = findMovableAncestor(selectedItem);
                                if (!root)
                                    continue;

                                if (_movingItems.contains(root))
                                    continue;

                                _movingItems.append(root);
                                _moveGroupInitialPos.append(root->scenePos());
                            }

                            if (!_movingItems.contains(_movingItem))
                            {
                                _movingItems.append(_movingItem);
                                _moveGroupInitialPos.append(_movingItem->scenePos());
                            }
                        }
                        else
                        {
                            // Одну фигуру тоже кладем в общий список
                            _movingItems.append(_movingItem);
                            _moveGroupInitialPos.append(_movingItem->scenePos());
                        }
                    }
                }

                _lastMousePos = mouseEvent->pos();
                mouseEvent->accept();
                updateModeLabel();
                return;
            }

            // Обычный ЛКМ - только перемещение изображения
            _isDraggingImage = true;
            _lastMousePos = mouseEvent->pos();
            mouseEvent->accept();
            updateModeLabel();
            return;
        }

        _lastMousePos = mouseEvent->pos();
    }
    if (_drawingCircle)
    {
        _isInDrawingMode = true;
        setSceneItemsMovable(false);
        updateModeLabel();
        if (!_isDrawingCircle)
        {
            _startPoint = graphView->mapToScene(mouseEvent->pos());
            if (!_currCircle)
            {
                _currCircle = new QGraphicsEllipseItem();
                QPen pen;
                pen.setWidthF(_vstyle.lineWidth);
                pen.setColor(_vstyle.circleLineColor);
                pen.setCosmetic(true);
                _currCircle->setPen(pen);
                if (_vstyle.fillShapeWhenSelected)
                {
                    QColor fill = _vstyle.circleLineColor;
                    fill.setAlpha(60);
                    _currCircle->setBrush(QBrush(fill));
                }
                else
                {
                    _currCircle->setBrush(Qt::NoBrush);
                }
                scene->addItem(_currCircle);
            }
            // Задаем позицию и начальный размер
            _currCircle->setRect(QRectF(_startPoint, QSizeF(1, 1)));
            _isDrawingCircle = true;

            const qreal crossSize = 6.0;
            if (!_currCircleCrossV)
            {
                _currCircleCrossV = scene->addLine(0, 0, 0, 0);
                _currCircleCrossV->setFlag(QGraphicsItem::ItemIsMovable, false);
                _currCircleCrossV->setFlag(QGraphicsItem::ItemIsSelectable, false);
                _currCircleCrossV->setFlag(QGraphicsItem::ItemIsFocusable, false);
                _currCircleCrossV->setAcceptedMouseButtons(Qt::NoButton);

                _currCircleCrossV->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
                _currCircleCrossV->setZValue(1e9);
            }
            if (!_currCircleCrossH)
            {
                _currCircleCrossH = scene->addLine(0, 0, 0, 0);
                _currCircleCrossH->setFlag(QGraphicsItem::ItemIsMovable, false);
                _currCircleCrossH->setFlag(QGraphicsItem::ItemIsSelectable, false);
                _currCircleCrossH->setFlag(QGraphicsItem::ItemIsFocusable, false);
                _currCircleCrossH->setAcceptedMouseButtons(Qt::NoButton);

                _currCircleCrossH->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
                _currCircleCrossH->setZValue(1e9);
            }
            QPen pen;
            pen.setWidthF(_vstyle.lineWidth);
            pen.setColor(_vstyle.circleLineColor);
            pen.setCosmetic(true);
            _currCircleCrossV->setPen(pen);
            _currCircleCrossH->setPen(pen);

            const qreal cx = _startPoint.x();
            const qreal cy = _startPoint.y();
            _currCircleCrossV->setPos(cx, cy);
            _currCircleCrossH->setPos(cx, cy);

            _currCircleCrossV->setLine(0, -crossSize, 0, +crossSize);
            _currCircleCrossH->setLine(-crossSize, 0, +crossSize, 0);
        }
        else if (_currCircle)
        {
            _isDrawingCircle = false;
        }
        _drawingCircle = false;
    }
    else if (_drawingPolyline)
    {
        _pendingDrawTool = PendingDrawTool::Polyline;
        _pendingDrawPressViewPos = mouseEvent->pos();
        _pendingDrawPressScenePos = graphView->mapToScene(mouseEvent->pos());

        _isInDrawingMode = true;
        setSceneItemsMovable(false);
        updateModeLabel();

        _lastMousePos = mouseEvent->pos();
        mouseEvent->accept();
        return;
    }
    else if (_drawingRectangle)
    {
        _isInDrawingMode = true;
        setSceneItemsMovable(false);
        updateModeLabel();
        if (!_isDrawingRectangle)
        {
            _startPoint = graphView->mapToScene(mouseEvent->pos()); // Сохраняем начальную точку

            QPointF startPoint2  = _startPoint;
            startPoint2.rx() += 2;
            startPoint2.ry() += 2;

            if (!_currRectangle)
            {
                _currRectangle = new QGraphicsRectItem();
                QPen pen;
                pen.setWidthF(_vstyle.lineWidth);
                pen.setColor(_vstyle.rectangleLineColor);
                pen.setCosmetic(true);
                _currRectangle->setPen(pen);
                if (_vstyle.fillShapeWhenSelected)
                {
                    QColor fill = _vstyle.rectangleLineColor;
                    fill.setAlpha(60);
                    _currRectangle->setBrush(QBrush(fill));
                }
                else
                {
                    _currRectangle->setBrush(Qt::NoBrush);
                }
                scene->addItem(_currRectangle);
            }
            _currRectangle->setRect(QRectF(_startPoint, startPoint2));
            _isDrawingRectangle = true; // Включаем флаг рисования
        }
        else if (_currRectangle)
        {
            _isDrawingRectangle = false;
        }
        _drawingRectangle = false;
    }
    else if (_drawingPoint)
    {
        _isInDrawingMode = true;
        setSceneItemsMovable(false);
        updateModeLabel();
        const QPointF sceneP = graphView->mapToScene(mouseEvent->pos());

        _currPoint = new qgraph::Point(scene);
        ensureUid(_currPoint);

        // Применяем стили из настроек
        apply_PointSize_ToItem(_currPoint);
        apply_NumberSize_ToItem(_currPoint);
        apply_PointStyle_ToItem(_currPoint);

        _currPoint->setCenter(sceneP);
        _currPoint->setFocus();

        SelectClass dialog(_projectClasses, this);

        if (dialog.exec() == QDialog::Accepted)
        {
            const QString selectedClass = dialog.selectedClass();
            if (!selectedClass.isEmpty())
            {
                _currPoint->setData(0, selectedClass);
                applyClassColorToItem(_currPoint, selectedClass);
                linkSceneItemToList(_currPoint);

                if (Document::Ptr doc = currentDocument())
                {
                    if (QUndoStack* stack = activeUndoStack())
                    {
                        stack->push(new undo::Create(doc.get(),
                                                     _currPoint,
                                                     u8"Добавление точки"));
                    }
                }
            }
        }

        _currPoint = nullptr;

        if (Document::Ptr doc = currentDocument())
        {
            doc->isModified = true;
            updateFileListDisplay(doc->filePath);
        }

        // Сразу завершаем, точка - единичный клик
        _drawingPoint = false;
        _isInDrawingMode = false;
        setSceneItemsMovable(true);
        updateModeLabel();
        raiseAllHandlesToTop();
        return;
    }
    else if (_drawingLine)
    {
        _pendingDrawTool = PendingDrawTool::Line;
        _pendingDrawPressViewPos  = mouseEvent->pos();
        _pendingDrawPressScenePos = graphView->mapToScene(mouseEvent->pos());

        _isInDrawingMode = true;
        setSceneItemsMovable(false);
        updateModeLabel();

        _lastMousePos = mouseEvent->pos();
        mouseEvent->accept();
        return;
    }
    else if (_drawingRuler)
    {
        if (!_isDrawingRuler)
        {
            // Первая точка линейки
            _rulerStartPoint = graphView->mapToScene(mouseEvent->pos());

            _isDrawingRuler = true;
            updateModeLabel();

            // Удаляем старую линейку (если осталась от прошлого измерения)
            if (_rulerLine)
            {
                scene->removeItem(_rulerLine);
                delete _rulerLine;
                _rulerLine = nullptr;
            }
            if (_rulerText)
            {
                scene->removeItem(_rulerText);
                delete _rulerText;
                _rulerText = nullptr;
            }

            QPen pen(_vstyle.rulerColor);
            pen.setWidthF(2.0);
            pen.setStyle(Qt::DotLine);
            pen.setCapStyle(Qt::RoundCap);
            pen.setJoinStyle(Qt::RoundJoin);
            pen.setCosmetic(true);

            _rulerLine = scene->addLine(QLineF(_rulerStartPoint, _rulerStartPoint), pen);
            _rulerLine->setZValue(1000);

            // Текст с расстоянием
            _rulerText = scene->addText(QStringLiteral("0"));
            _rulerText->setDefaultTextColor(_vstyle.rulerColor);
            // Масштабируем размер текста от размера изображения
            if (videoRect)
            {
                const QSize imgSize = videoRect->pixmap().size();
                const int maxSide = std::max(imgSize.width(), imgSize.height());

                QFont rulerTextFont = _rulerText->font();
                rulerTextFont.setPointSizeF(std::max(6.0, maxSide / 200.0));
                _rulerText->setFont(rulerTextFont);
            }
            _rulerText->setZValue(1001);
            _rulerText->setPos(_rulerStartPoint);
        }
        _drawingRuler = false;

        mouseEvent->accept();
        return;
    }
    //graphView->mousePressEvent(mouseEvent);
}

void MainWindow::graphicsView_mouseMoveEvent(QMouseEvent* mouseEvent, GraphicsView* graphView)
{
    // Если в рисовании Line/Polyline ЛКМ уехала дальше порога
    if (_pendingDrawTool != PendingDrawTool::None
        && (mouseEvent->buttons() & Qt::LeftButton)
        && graphView)
    {
        const int dragDist = QApplication::startDragDistance();
        if ((mouseEvent->pos() - _pendingDrawPressViewPos).manhattanLength() >= dragDist)
        {
            // Не постановка новой точки, а перетаскивание изображения
            _pendingDrawTool = PendingDrawTool::None;

            _movingItem = nullptr;
            _movingItems.clear();
            _moveGroupInitialPos.clear();
            _moveIsGroup = false;
            _moveHadChanges = false;

            _draggingItem = nullptr;
            _shiftImageDragging = false;

            _isDraggingImage = true;
            _lastMousePos = mouseEvent->pos();

            updateModeLabel();
        }
    }

    // Зум прямоугольной области Ctrl + ЛКМ
    if (_zoomRectActive && graphView && (mouseEvent->buttons() & Qt::LeftButton))
    {
        const int dragDist = QApplication::startDragDistance();
        const bool dragged =
            (mouseEvent->pos() - _zoomRectOrigin).manhattanLength() >= dragDist;

        if (dragged)
        {
            if (!_zoomRubberBand)
                _zoomRubberBand = new QRubberBand(QRubberBand::Rectangle, graphView->viewport());

            if (_zoomRectView)
                _zoomRectView->setInteractive(false);

            _zoomRubberBand->setGeometry(QRect(_zoomRectOrigin, mouseEvent->pos()).normalized());
            _zoomRubberBand->show();

            mouseEvent->accept();
            return;
        }
    }
    if (_isDrawingRuler && _rulerLine)
    {
       QPointF currentPoint = graphView->mapToScene(mouseEvent->pos());
       QLineF line(_rulerStartPoint, currentPoint);
       _rulerLine->setLine(line);

       const double dist = line.length();

       if (_rulerText)
       {
           QString text = QString::number(dist, 'f', 1);
           _rulerText->setPlainText(text);

           QPointF middle = (line.p1() + line.p2()) / 2.0;
           _rulerText->setPos(middle);
       }

       mouseEvent->accept();
       return;
    }

    if (_isInDrawingMode)
    {
        if (_isDrawingRectangle && _currRectangle)
        {
            // Обновляем размеры прямоугольника, пока пользователь тянет мышь
            QRectF newRect(_startPoint, graphView->mapToScene(mouseEvent->pos()));
            _currRectangle->setRect(newRect.normalized());
            // normalized: приводит координаты к нужному порядку (левый верхний угол - начальный)
            return;
        }
        else if (_isDrawingCircle && _currCircle)
        {
            QPointF currentPoint = graphView->mapToScene(mouseEvent->pos());
            // Вычисляем радиус
            qreal radius = QLineF(_startPoint, currentPoint).length();
            // Создаем квадрат для описания окружности, используя радиус
            QRectF circleRect(_startPoint.x() - radius,
                              _startPoint.y() - radius,
                              radius * 2, radius * 2);
            // Устанавливаем обновленный прямоугольник
            _currCircle->setRect(circleRect);
            if (_currCircleCrossV && _currCircleCrossH)
            {
                const qreal viewScale = graphView->transform().m11();
                const qreal radiusPx = radius * viewScale;

                const qreal baseLenPx = 6.0;
                const qreal maxLenPx = qMax<qreal>(1.0, radiusPx - 1.0);
                const qreal lenPx = qMin(baseLenPx, maxLenPx);

                const qreal cx = _startPoint.x();
                const qreal cy = _startPoint.y();

                _currCircleCrossV->setPos(cx, cy);
                _currCircleCrossH->setPos(cx, cy);

                _currCircleCrossV->setLine(0, -lenPx, 0, +lenPx);
                _currCircleCrossH->setLine(-lenPx, 0, +lenPx, 0);
            }
            return;
        }
        //return;
    }
    if (_movingItem && (mouseEvent->buttons() & Qt::LeftButton))
    {
        const QPointF scenePos = graphView->mapToScene(mouseEvent->pos());

        if (!_movingItems.isEmpty() && !_moveGroupInitialPos.isEmpty())
        {
            QPointF shift;

            if (_moveIsGroup)
            {
                // Для группы все фигуры сдвигаются одинаково
                // относительно положения курсора в момент захвата
                shift = scenePos - _movePressScenePos;
            }
            else
            {
                // Для одной фигуры учитываем место захвата внутри фигуры,
                // чтобы она не прыгала под курсор
                const QPointF targetPos = scenePos - _moveGrabOffsetScene;
                shift = targetPos - _moveGroupInitialPos.first();
            }

            const int count = qMin(_movingItems.size(), _moveGroupInitialPos.size());

            for (int i = 0; i < count; ++i)
            {
                QGraphicsItem* item = _movingItems[i];

                if (!item)
                    continue;

                item->setPos(_moveGroupInitialPos[i] + shift);
            }
        }

        _moveHadChanges = true;
        mouseEvent->accept();
        return;
    }
    else if (_movingItem)
    {
        _movingItem->setFlags(_moveSavedFlags);

        _movingItem = nullptr;
        _movingItems.clear();
        _moveGroupInitialPos.clear();
        _moveIsGroup = false;
        _moveHadChanges = false;
        _moveInProgress = false;

        mouseEvent->accept();
        updateModeLabel();
        return;
    }

    if (mouseEvent->buttons() & Qt::LeftButton)
    {
        if (_isDraggingImage)
        {
            const QPoint deltaView = mouseEvent->pos() - _lastMousePos;

            qreal sx = graphView->transform().m11();
            qreal sy = graphView->transform().m22();

            if (std::abs(sx) < 0.000001) sx = 1.0;
            if (std::abs(sy) < 0.000001) sy = 1.0;

            graphView->setTransformationAnchor(QGraphicsView::NoAnchor);
            graphView->translate(deltaView.x() / sx, deltaView.y() / sy);
            graphView->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

            _lastMousePos = mouseEvent->pos();
            mouseEvent->accept();
            return;
        }
        else if (_draggingItem && (mouseEvent->modifiers() & Qt::ShiftModifier))
        {
            // Смещение в системе координат сцены
            const QPointF curScene = graphView->mapToScene(mouseEvent->pos());
            const QPointF prevScene = graphView->mapToScene(_lastMousePos);
            const QPointF delta = curScene - prevScene;

            _draggingItem->moveBy(delta.x(), delta.y());

            // Обновляем ручки для перемещаемой фигуры
            if (qgraph::Circle* circle = dynamic_cast<qgraph::Circle*>(_draggingItem))
                circle->updateHandlePosition();
            else if (qgraph::Rectangle* rect = dynamic_cast<qgraph::Rectangle*>(_draggingItem))
                rect->updateHandlePosition();
            else if (qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(_draggingItem))
                polyline->updateHandlePosition();
            else if (qgraph::Line* line = dynamic_cast<qgraph::Line*>(_draggingItem))
                line->updateHandlePosition();

            updateAllPointNumbers();
        }

        _lastMousePos = mouseEvent->pos();
    }
    _lastMousePos = mouseEvent->pos();
}

void MainWindow::graphicsView_mouseReleaseEvent(QMouseEvent* mouseEvent, GraphicsView* graphView)
{
    Document::Ptr doc = currentDocument();

    if (mouseEvent->button() == Qt::LeftButton)
        _leftMouseButtonDown = false;
    // Если не сдвиг, ставим точку
    if (mouseEvent->button() == Qt::LeftButton
        && _pendingDrawTool != PendingDrawTool::None
        && graphView)
    {
        const QPointF scenePos = _pendingDrawPressScenePos;
        const Qt::KeyboardModifiers mods = mouseEvent->modifiers();
        const PendingDrawTool tool = _pendingDrawTool;
        _pendingDrawTool = PendingDrawTool::None;

        if (tool == PendingDrawTool::Polyline)
            handlePolylineLmbClick(scenePos, mods, graphView);
        else if (tool == PendingDrawTool::Line)
            handleLineLmbClick(scenePos, mods, graphView);

        mouseEvent->accept();
        return;
    }

    // Зум прямоугольной области Ctrl + ЛКМ
    if (_zoomRectActive && graphView && mouseEvent->button() == Qt::LeftButton)
    {
        _zoomRectActive = false;
        updateModeLabel();

        QRect viewRect;
        const bool hadRubberBand = (_zoomRubberBand && _zoomRubberBand->isVisible());

        if (_zoomRubberBand)
        {
            viewRect = _zoomRubberBand->geometry().normalized();
            _zoomRubberBand->hide();
        }

        if (_zoomRectView)
        {
            _zoomRectView->setInteractive(_zoomRectWasInteractive);
            _zoomRectView.clear();
        }

        if (hadRubberBand && viewRect.width() >= 10 && viewRect.height() >= 10)
        {
            const QRectF sceneRect = graphView->mapToScene(viewRect).boundingRect();

            graphView->fitInView(sceneRect, Qt::KeepAspectRatio);
            graphView->centerOn(sceneRect.center());

            _m_zoom = graphView->transform().m11();
            if (_m_zoom <= 0.000001)
                _m_zoom = 1.0;

            if (Document::Ptr doc = currentDocument())
                saveCurrentViewState(doc);

            updateAllPointNumbers();
            graphView->viewport()->update();

            mouseEvent->accept();
            return;
        }

        // Если протяжки не было, это обычный Ctrl+клик по фигуре для мультивыделения
        if ((mouseEvent->modifiers() & Qt::ControlModifier) &&
            !(mouseEvent->modifiers() & Qt::ShiftModifier))
        {
            QGraphicsItem* clickedItem = graphView->itemAt(_zoomRectOrigin);
            if (!clickedItem)
                clickedItem = pickItemByEdgeAt(graphView, _zoomRectOrigin);

            QGraphicsItem* selectionItem = clickedItem;

            if (qgraph::DragCircle* handle = dynamic_cast<qgraph::DragCircle*>(selectionItem))
                selectionItem = handle->parentItem();

            if (selectionItem && selectionItem != doc->videoRect)
                selectionItem = findMovableAncestor(selectionItem);

            if (selectionItem && selectionItem != doc->videoRect)
            {
                selectionItem->setSelected(!selectionItem->isSelected());
                mouseEvent->accept();
                return;
            }
        }

        mouseEvent->accept();
        return;
    }

    // Перемещение изображения - убрали, пока не надо
    // if (_shiftImageDragging && mouseEvent->button() == Qt::LeftButton)
    // {
    //     _shiftImageDragging = false;
    //     updateModeLabel();
    //     if (_videoRect)
    //     {
    //         QPointF after = _videoRect->pos();
    //         pushMoveImageCommand(_shiftImageBeforePos,
    //                              after,
    //                              u8"Перемещение изображения"));
    //     }
    //     updateModeLabel();
    //     mouseEvent->accept();
    // }

    if (mouseEvent->button() == Qt::LeftButton)
    {
        _isDraggingImage = false;
        _draggingItem    = nullptr;

        const bool hasOpenLine = _line && !_line->isClosed();

        const bool hasOpenPolyline = _polyline && !_polyline->isClosed();

        const bool keepLineLikeDrawing = _drawingPolyline
                                        || _drawingLine
                                        || _isDrawingPolyline
                                        || _isDrawingLine
                                        || hasOpenLine
                                        || hasOpenPolyline;

        if (_isInDrawingMode)
        {
            if (keepLineLikeDrawing)
            {
                setSceneItemsMovable(false);
            }
            else
            {
                _isInDrawingMode = false;
                setSceneItemsMovable(true);
            }
        }
        updateModeLabel();
    }
    if (mouseEvent->button() == Qt::LeftButton)
    {
        if (_movingItem)
        {
            _movingItem->setFlags(_moveSavedFlags);

            if (_moveHadChanges && !_movingItems.isEmpty())
            {
                const int count = qMin(_movingItems.size(), _moveGroupInitialPos.size());

                QPointF delta;
                QSet<QGraphicsItem*> shapes;
                bool hasMovedShapes = false;

                for (int i = 0; i < count; ++i)
                {
                    QGraphicsItem* item = _movingItems[i];

                    if (!item)
                        continue;

                    if (!hasMovedShapes)
                    {
                        delta = item->scenePos() - _moveGroupInitialPos[i];

                        if (qFuzzyIsNull(delta.x()) && qFuzzyIsNull(delta.y()))
                            break;

                        hasMovedShapes = true;
                    }
                    shapes.insert(item);
                }

                if (hasMovedShapes && shapes.count())
                    if (Document::Ptr doc = currentDocument())
                    {
                        if (QUndoStack* stack = activeUndoStack())
                        {
                            const QString description = (shapes.count() == 1)
                                                        ? u8"Перемещение фигуры"
                                                        : u8"Перемещение фигур";

                            stack->push(new undo::Move(doc.get(), shapes, description, delta));
                        }
                        doc->isModified = true;
                        updateFileListDisplay(doc->filePath);
                    }
            }
            _movingItem = nullptr;
            _movingItems.clear();
            _moveGroupInitialPos.clear();
            _moveIsGroup = false;
            _moveHadChanges = false;
            _moveInProgress = false;

            updateCoordinateList();
            mouseEvent->accept();
            updateModeLabel();
            return;
        }
    }
    if (mouseEvent->button() == Qt::LeftButton && _isDrawingRectangle)
    {
        _isDrawingRectangle = false;
        // Получаем финальный прямоугольник и нормализуем его (если размеры отрицательны)
        QRectF finalRect = _currRectangle->sceneBoundingRect();
        // Убираем временный прямоугольник из сцены
        doc->scene->removeItem(_currRectangle);
        delete _currRectangle;
        _currRectangle = nullptr; // Сбрасываем ссылку, чтобы начать новый прямоугольник

        // Проверяем минимальный размер
        if (finalRect.width() < 1 || finalRect.height() < 1)
            return; // Не создаем объект, если размеры некорректны

        // Создаем объект класса Rectangle
        qgraph::Rectangle* rectangle = new qgraph::Rectangle(doc->scene);
        ensureUid(rectangle);
        rectangle->setRealSceneRect(finalRect); // Устанавливаем координаты и размеры
        rectangle->updatePointNumbers();
        apply_LineWidth_ToItem(rectangle);
        apply_PointSize_ToItem(rectangle);
        apply_NumberSize_ToItem(rectangle);

        SelectClass dialog(_projectClasses, this);

        if (dialog.exec() == QDialog::Accepted)
        {
            QString selectedClass = dialog.selectedClass();
            if (!selectedClass.isEmpty())
            {
                rectangle->setData(0, selectedClass);
                applyClassColorToItem(rectangle, selectedClass);
                linkSceneItemToList(rectangle); // Связываем новый элемент с списком
            }
        }
        if (Document::Ptr doc = currentDocument())
        {
            if (QUndoStack* stack = activeUndoStack())
            {
                stack->push(new undo::Create(doc.get(), rectangle,
                                             u8"Добавлен прямоугольник"));
            }
            doc->isModified = true;
            updateFileListDisplay(doc->filePath);
        }
        raiseAllHandlesToTop();
    }
    else if (mouseEvent->button() == Qt::LeftButton && _isDrawingCircle)
    {
        _isDrawingCircle = false;
        // Получаем финальный прямоугольник круга
        QRectF circleRect = _currCircle->rect();

        // Удаляем временный круг
        doc->scene->removeItem(_currCircle);
        delete _currCircle;
        _currCircle = nullptr;

        // Удаляем временный крестик центра
        if (_currCircleCrossV)
        {
            doc->scene->removeItem(_currCircleCrossV);
            delete _currCircleCrossV;
            _currCircleCrossV = nullptr;
        }
        if (_currCircleCrossH)
        {
            doc->scene->removeItem(_currCircleCrossH);
            delete _currCircleCrossH;
            _currCircleCrossH = nullptr;
        }

        // Проверяем минимальный радиус
        if (circleRect.width() < 1 || circleRect.height() < 1)
            return; // Прерываем, если круг слишком маленький


        // Создаем объект класса Circle
        qgraph::Circle* circle = new qgraph::Circle(doc->scene, _startPoint);
        ensureUid(circle);
        // Устанавливаем радиус финального круга
        qreal finalRadius = circleRect.width() / 2;
        circle->setRealRadius(finalRadius);
        apply_LineWidth_ToItem(circle);
        apply_PointSize_ToItem(circle);

        SelectClass dialog(_projectClasses, this);

        if (dialog.exec() == QDialog::Accepted)
        {
            QString selectedClass = dialog.selectedClass();
            if (!selectedClass.isEmpty())
            {
                circle->setData(0, selectedClass);
                applyClassColorToItem(circle, selectedClass);
                linkSceneItemToList(circle); // Связываем новый элемент с списком

                if (Document::Ptr doc = currentDocument())
                {
                    if (QUndoStack* stack = activeUndoStack())
                    {
                        stack->push(new undo::Create(doc.get(),
                                                     circle,
                                                     u8"Добавление круга"));
                    }
                }

            }
        }
        if (Document::Ptr doc = currentDocument())
        {
            doc->isModified = true;
            updateFileListDisplay(doc->filePath);
        }
        raiseAllHandlesToTop();
    }
    else if (mouseEvent->button() == Qt::LeftButton && _isDrawingRuler)
    {
        // Фиксируем конечную точку и окончательно считаем расстояние
        _isDrawingRuler = false;
        updateModeLabel();

        if (_rulerLine)
        {
            QLineF line = _rulerLine->line();
            const double dist = line.length();

            if (_rulerText)
            {
                QString text = QString::number(dist, 'f', 1);
                _rulerText->setPlainText(text);

                QPointF middle = (line.p1() + line.p2()) / 2.0;
                _rulerText->setPos(middle);
            }
        }

        mouseEvent->accept();
        return;
    }

    if (mouseEvent->button() == Qt::LeftButton && _isAllMoved)
    {
        _isAllMoved = false;
        updateModeLabel();
        return;
    }

    if (mouseEvent->button() == Qt::LeftButton)
        _moveInProgress = false;

    //graphView->mouseReleaseEvent(mouseEvent);
}

void MainWindow::setSceneItemsMovable(bool movable)
{
    Document::Ptr doc = currentDocument();
    if (!doc || !doc->scene)
        return;
    for (QGraphicsItem* item : doc->scene->items())
    {
        if (!item)
            continue;

        // Не трогаем дочерние элементы (ручки, контуры)
        if (item->parentItem() != nullptr)
            continue;

        if (item != doc->videoRect
            && item != _tempRectItem
            && item != _tempCircleItem
            && item != _tempPolyline
            && item != _currCircle
            && item != _currCircleCrossV
            && item != _currCircleCrossH)
        {
            item->setFlag(QGraphicsItem::ItemIsMovable, movable);
        }
    }
}

Document::Ptr MainWindow::currentDocument() const
{
    if (QListWidgetItem* currentItem = ui->fileList->currentItem())
    {
        QVariant data = currentItem->data(Qt::UserRole);
        if (data.isValid() && data.canConvert<Document::Ptr>())
            return data.value<Document::Ptr>();
    }
    return {};
}

void MainWindow::changeClassByUid(qulonglong uid)
{
    Document::Ptr doc = currentDocument();

    if (!doc->scene || uid == 0)
        return;

    QGraphicsItem* target = nullptr;

    for (QGraphicsItem* item : doc->scene->items())
    {
        if (item == nullptr
            || (item == doc->videoRect)
            || (item == _tempRectItem)
            || (item == _tempCircleItem)
            || (item == _tempPolyline))
        {
            continue;
        }

        if (item->data(_roleUid).toULongLong() == uid)
        {
            target = item;
            break;
        }
    }
    if (!target)
        return;

    changeClassForSceneItem(target);
}

void MainWindow::toggleSceneItemVisibility(QGraphicsItem* item)
{
    if (!item)
        return;

    Document::Ptr doc = currentDocument();

    if (qgraph::DragCircle* handle = dynamic_cast<qgraph::DragCircle*>(item))
        item = handle->parentItem();

    if (!item || item == doc->videoRect)
        return;

    item = findMovableAncestor(item);
    if (!item)
        return;

    QList<QGraphicsItem*> targets;

    // Сначала пытаемся собрать группу из выделения в правом списке,
    // так как команда вызывается из контекстного меню polygonList
    QModelIndexList selectedRows;

    if (ui && ui->polygonList && ui->polygonList->selectionModel())
        selectedRows = ui->polygonList->selectionModel()->selectedRows();

    bool clickedItemIsInListSelection = false;

    for (const QModelIndex& index : selectedRows)
    {
        if (!index.isValid())
            continue;

        if (!doc)
            continue;

        const int row = index.row();

        if (row < 0 || row >= doc->polygonList.items.size())
            continue;

        QGraphicsItem* selectedSceneItem = doc->polygonList.items[row];

        if (!selectedSceneItem)
            continue;

        if (qgraph::DragCircle* handle = dynamic_cast<qgraph::DragCircle*>(selectedSceneItem))
            selectedSceneItem = handle->parentItem();

        selectedSceneItem = findMovableAncestor(selectedSceneItem);

        if (!selectedSceneItem || selectedSceneItem == doc->videoRect)
            continue;

        if (selectedSceneItem == item)
            clickedItemIsInListSelection = true;
    }

    if (clickedItemIsInListSelection)
    {
        for (const QModelIndex& index : selectedRows)
        {
            if (!index.isValid())
                continue;

            if (!doc)
                continue;

            const int row = index.row();

            if (row < 0 || row >= doc->polygonList.items.size())
                continue;

            QGraphicsItem* selectedSceneItem = doc->polygonList.items[row];

            if (!selectedSceneItem)
                continue;

            if (qgraph::DragCircle* handle = dynamic_cast<qgraph::DragCircle*>(selectedSceneItem))
                selectedSceneItem = handle->parentItem();

            selectedSceneItem = findMovableAncestor(selectedSceneItem);

            if (!selectedSceneItem || selectedSceneItem == doc->videoRect)
                continue;

            if (!targets.contains(selectedSceneItem))
                targets.append(selectedSceneItem);
        }
    }
    else
    {
        targets.append(item);
    }

    if (targets.isEmpty())
        return;

    bool hideMode = false;
    for (QGraphicsItem* target : targets)
    {
        if (target && target->isVisible())
        {
            hideMode = true;
            break;
        }
    }

    const QString description =
        hideMode
            ? ((targets.size() == 1) ? u8"Скрытие фигуры" : u8"Скрытие фигур")
            : ((targets.size() == 1) ? u8"Показ фигуры"   : u8"Показ фигур");

    QUndoStack* stack = activeUndoStack();

    if (!stack)
        return;

    QVector<QGraphicsItem*> shapes;

    for (QGraphicsItem* target : targets)
    {
        if (!target)
            continue;

        target->setVisible(!hideMode);

        if (hideMode)
            target->setSelected(false);

        shapes.append(target);
    }

    if (shapes.isEmpty())
        return;

    stack->push(new undo::Visibility(doc.get(),
                                     shapes,
                                     !hideMode,
                                     description));

    updatePolygonListTexts();

    if (Document::Ptr doc = currentDocument())
    {
        doc->isModified = true;
        updateFileListDisplay(doc->filePath);
    }
    updateCoordinateList();

    if (doc->scene)
        doc->scene->update();
}

bool MainWindow::eventFilter(QObject* obj, QEvent* event)
{
    if (obj != ui->graphView->viewport())
        return QMainWindow::eventFilter(obj, event);

    switch (event->type())
    {
        case QEvent::Leave:
        {
            clearAllHandleHoverEffects();
            hideGhost();
            break;
        }
        case QEvent::MouseButtonDblClick:
        {
            QMouseEvent* me = static_cast<QMouseEvent*>(event);

            if (me->button() != Qt::LeftButton)
                break;

            if (_drawingPolyline
                && _polyline
                && qgraph::Polyline::globalCloseMode() == qgraph::Polyline::CloseMode::DoubleClick)
            {
                _pendingDrawTool = PendingDrawTool::None;

                if (!_polyline->isClosed())
                    _polyline->closePolyline();

                event->accept();
                return true;
            }

            if (_drawingLine
                && _line
                && qgraph::Line::globalCloseMode() == qgraph::Line::CloseMode::DoubleClick)
            {
                _pendingDrawTool = PendingDrawTool::None;

                if (!_line->isClosed())
                    _line->closeLine();

                event->accept();
                return true;
            }

            break;
        }
        case QEvent::MouseButtonPress:
        {
            QMouseEvent* me = static_cast<QMouseEvent*>(event);
            if (me->button() != Qt::LeftButton)
                break;

            const QPointF sp = ui->graphView->mapToScene(me->pos());

            if (_drawingRuler || _isDrawingRuler)
            {
                clearAllHandleHoverEffects();
                hideGhost();
                break;
            }
            // Alt + Shift + ЛКМ удалить узел
            if ((me->modifiers() & Qt::AltModifier) && (me->modifiers() & Qt::ShiftModifier))
            {
                qgraph::DragCircle* handle = pickHandleAt(sp);

                if (!handle)
                {
                    bool topIsHandle = false;
                    handle = pickHiddenHandle(sp, topIsHandle);
                }

                if (handle && handle->parentItem())
                {
                    if (qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(handle->parentItem()))
                    {
                        const QVector<qgraph::DragCircle*>& circles = polyline->circles();
                        const int pointIndex = circles.indexOf(handle);

                        if (pointIndex < 0)
                        {
                            event->accept();
                            return true;
                        }

                        const QPointF point = handle->scenePos();

                        if (_currentHoveredHandle == handle)
                            _currentHoveredHandle = nullptr;

                        if (_lastHoverHandle == handle)
                            _lastHoverHandle = nullptr;

                        if (_ghostTarget == handle)
                        {
                            _ghostTarget = nullptr;

                            if (_ghostHandle)
                                _ghostHandle->setVisible(false);
                        }

                        if (_m_dragHandle == handle)
                        {
                            _m_dragHandle = nullptr;
                            _m_isDraggingHandle = false;
                        }

                        if (polyline->removePointAtIndex(pointIndex))
                        {
                            if (QUndoStack* stack = activeUndoStack())
                            {
                                undo::NodeEdit::Data data;
                                data.type = undo::NodeEdit::Type::RemovePolylineNode;
                                data.pointIndex = pointIndex;
                                data.point = point;

                                stack->push(new undo::NodeEdit(currentDocument().get(),
                                                               polyline,
                                                               data,
                                                               u8"Удаление узла"));
                            }

                            if (Document::Ptr doc = currentDocument(); doc && !doc->isModified)
                            {
                                doc->isModified = true;
                                updateFileListDisplay(doc->filePath);
                            }
                        }

                        event->accept();
                        return true;
                    }

                    if (qgraph::Line* line = dynamic_cast<qgraph::Line*>(handle->parentItem()))
                    {
                        const QVector<qgraph::DragCircle*>& circles = line->circles();
                        const int pointIndex = circles.indexOf(handle);

                        if (pointIndex < 0)
                        {
                            event->accept();
                            return true;
                        }

                        const QPointF point = handle->scenePos();

                        if (_currentHoveredHandle == handle)
                            _currentHoveredHandle = nullptr;

                        if (_lastHoverHandle == handle)
                            _lastHoverHandle = nullptr;

                        if (_ghostTarget == handle)
                        {
                            _ghostTarget = nullptr;

                            if (_ghostHandle)
                                _ghostHandle->setVisible(false);
                        }

                        if (_m_dragHandle == handle)
                        {
                            _m_dragHandle = nullptr;
                            _m_isDraggingHandle = false;
                        }

                        if (line->removePointAtIndex(pointIndex))
                        {
                            if (QUndoStack* stack = activeUndoStack())
                            {
                                undo::NodeEdit::Data data;
                                data.type = undo::NodeEdit::Type::RemoveLineNode;
                                data.pointIndex = pointIndex;
                                data.point = point;

                                stack->push(new undo::NodeEdit(currentDocument().get(),
                                                               line,
                                                               data,
                                                               u8"Удаление узла"));
                            }

                            if (Document::Ptr doc = currentDocument(); doc && !doc->isModified)
                            {
                                doc->isModified = true;
                                updateFileListDisplay(doc->filePath);
                            }
                        }

                        event->accept();
                        return true;
                    }
                }
            }
            // Shift + ЛКМ используется для перемещения всей фигуры
            // Поэтому ручки не должны перехватывать этот жест как редактирование узла
            if (me->modifiers() & Qt::ShiftModifier)
                break;
            // Если мы были в режиме "продолжение" и пользователь откатился undo
            // до состояния, где фигуры уже нет
            if (_resumeEditing && _resumeUid)
            {
                if (!findItemByUid(_resumeUid))
                {
                    _resumeEditing = false;
                    _resumeUid = 0;
                    _drawingLine = false;
                    _drawingPolyline = false;
                    _line = nullptr;
                    _polyline = nullptr;
                    setSceneItemsMovable(true);
                    updateModeLabel();
                }
            }
            // Соединение лиинй
            if (_mergeLinesMode)
            {
                if (handleMergeLinesClick(sp))
                {
                    event->accept();
                    return true;
                }
            }
            bool topIsHandle = false;

            if (_drawingPolyline && _polyline && !_polyline->isClosed() &&
                qgraph::Polyline::globalCloseMode() == qgraph::Polyline::CloseMode::SingleClickOnFirstPoint)
            {
                const QVector<qgraph::DragCircle*>& circles = _polyline->circles();
                if (!circles.isEmpty())
                {
                    const QVector<qgraph::DragCircle*>& circles = _polyline->circles();

                    if (!circles.isEmpty())
                    {
                        qgraph::DragCircle* firstHandle = circles.first();
                        qgraph::DragCircle* clickedHandle = pickHandleAt(sp);

                        if (clickedHandle && clickedHandle == firstHandle)
                        {
                            _pendingDrawTool = PendingDrawTool::None;
                            _polyline->closePolyline();

                            event->accept();
                            return true;
                        }
                    }
                }
            }

            Document::Ptr doc = currentDocument();

            const bool drawingLineLike =
                    _drawingPolyline || _drawingLine || _isDrawingPolyline || _isDrawingLine;

            if (drawingLineLike)
            {
                const bool finishPolylineByCtrl =
                        _drawingPolyline
                        && _polyline
                        && qgraph::Polyline::globalCloseMode() == qgraph::Polyline::CloseMode::CtrlModifier
                        && (me->modifiers() & Qt::ControlModifier);

                const bool finishLineByCtrl =
                        _drawingLine
                        && _line
                        && qgraph::Line::globalCloseMode() == qgraph::Line::CloseMode::CtrlModifier
                        && (me->modifiers() & Qt::ControlModifier);

                if (!finishPolylineByCtrl && !finishLineByCtrl)
                {
                    if (qgraph::DragCircle* preciseHandle = pickHandleAt(sp, _drawHandleCommitRadius))
                    {
                        if (preciseHandle->scene() && doc->scene && doc->scene->items().contains(preciseHandle))
                        {
                            startHandleDrag(preciseHandle, sp);
                            return true;
                        }
                    }
                }

                break; // пусть клик дойдет до handleLineLmbClick / handlePolylineLmbClick
            }

            if (qgraph::DragCircle* target = pickHiddenHandle(sp, topIsHandle))
            {
                if (target->scene() && doc->scene && doc->scene->items().contains(target))
                {
                    if (topIsHandle)
                    {
                        startHandleDrag(target, sp);
                    }
                    else
                    {
                        showGhostFor(target);
                        startGhostDrag(sp);
                    }
                    return true;
                }
            }
            break;
        }
        case QEvent::MouseMove:
        {
            if (_movingItem || _moveIsGroup || _isDraggingImage)
            {
                if (_currentHoveredHandle && _currentHoveredHandle->isValid())
                    _currentHoveredHandle->setHoverStyle(false);

                _currentHoveredHandle = nullptr;
                hideGhost();
                break;
            }

            QMouseEvent* me = static_cast<QMouseEvent*>(event);
            const QPointF sp = ui->graphView->mapToScene(me->pos());

            if (_drawingRuler || _isDrawingRuler)
            {
                clearAllHandleHoverEffects();
                hideGhost();
                break;
            }
            if (_m_isDraggingHandle && _m_dragHandle)
            {
                // Проверяем валидность указателя перед обновлением
                if (_m_dragHandle && _m_dragHandle->scene())
                {
                    updateHandleDrag(sp);
                    return true;
                }
                else
                {
                    // Если указатель невалиден, сбрасываем состояние
                    finishHandleDrag();
                }
            }
            else if (_ghostTarget && _ghostHandle && _ghostHandle->isVisible())
            {
                if (_ghostTarget->scene())
                {
                    moveGhostTo(sp);
                    return true;
                }
                else
                {
                    hideGhost();
                }
            }
            else
            {
                // Ищем ручку под курсором
                qgraph::DragCircle* hoveredHandle = pickHandleAt(sp);

                if (hoveredHandle && !hoveredHandle->isValid())
                {
                    hoveredHandle = nullptr;
                }

                // Если нашли новую ручку, отличающуюся от текущей
                if (hoveredHandle != _currentHoveredHandle)
                {
                    // Убираем hover с предыдущей ручки
                        if (_currentHoveredHandle && _currentHoveredHandle->isValid())
                    {
                        _currentHoveredHandle->setHoverStyle(false);
                    }
                    // Устанавливаем hover на новую ручку
                    if (hoveredHandle && hoveredHandle->isValid())
                    {
                        hoveredHandle->setHoverStyle(true);
                    }

                    _currentHoveredHandle = hoveredHandle;
                }
            }
            break;
        }
        case QEvent::MouseButtonRelease:
        {
            QMouseEvent* me = static_cast<QMouseEvent*>(event);
            if (me->button() != Qt::LeftButton)
                break;

            if (_m_isDraggingHandle && _m_dragHandle)
            {
                finishHandleDrag();
                return true;
            }
            else if (_ghostTarget && _ghostHandle && _ghostHandle->isVisible())
            {
                if (_ghostTarget->scene())
                {
                    endGhost();
                }
                else
                {
                    hideGhost();
                }
                return true;
            }
            break;
        }
        case QEvent::ShortcutOverride:
        case QEvent::KeyPress:
        {
            QKeyEvent* ke = static_cast<QKeyEvent*>(event);

            if (ke->key() == Qt::Key_C)
            {
                const bool closePolylineByKey =
                        _drawingPolyline
                        && _polyline
                        && qgraph::Polyline::globalCloseMode() == qgraph::Polyline::CloseMode::KeyC;

                if (closePolylineByKey)
                {
                    if (event->type() == QEvent::KeyPress && !_polyline->isClosed())
                        _polyline->closePolyline();

                    event->accept();
                    return true;
                }

                const bool closeLineByKey =
                        _drawingLine
                        && _line
                        && qgraph::Line::globalCloseMode() == qgraph::Line::CloseMode::KeyC;

                if (closeLineByKey)
                {
                    if (event->type() == QEvent::KeyPress && !_line->isClosed())
                        _line->closeLine();

                    event->accept();
                    return true;
                }
            }

            if (_mergeLinesMode && ke->key() == Qt::Key_Escape)
            {
                cancelMergeLinesMode();
                event->accept();
                return true;
            }

            break;
        }
        case QEvent::Wheel:
        {
            if (obj != ui->graphView->viewport())
                break;

            QWheelEvent* we = static_cast<QWheelEvent*>(event);
            // Shift + колесико = горизонтальный скролл
            if ((we->modifiers() & Qt::ShiftModifier) &&
                !(we->modifiers() & Qt::ControlModifier))
            {
                int dx = we->angleDelta().y();
                if (dx == 0)
                    dx = we->pixelDelta().y();
                if (dx == 0)
                    dx = we->angleDelta().x();
                if (dx == 0)
                    dx = we->pixelDelta().x();

                if (dx != 0)
                {
                    QScrollBar* hbar = ui->graphView->horizontalScrollBar();
                    hbar->setValue(hbar->value() - dx);

                    if (Document::Ptr doc = currentDocument())
                        saveCurrentViewState(doc);
                }
                we->accept();
                return true;
            }
            // Ctrl + колесико = зум
            if (obj != ui->graphView->viewport())
                break;

            if (!(we->modifiers() & Qt::ControlModifier))
                break;

            // На мыши angleDelta, на тачпаде pixelDelta
            int dy = we->angleDelta().y();
            if (dy == 0)
                dy = we->pixelDelta().y();

            if (dy == 0)
                return true;

            // Переводим в "шаги колеса": 120 = один щелчок
            const double steps = dy / 120.0;

            const double kZoomStep = MainWindow::_kZoomStep;
            const double kZoomMin = MainWindow::_kMinZoom;
            const double kZoomMax = MainWindow::_kMaxZoom;

            double oldZoom = _m_zoom;
            if (oldZoom <= 0.000001)
                oldZoom = ui->graphView->transform().m11();
            if (oldZoom <= 0.000001)
                oldZoom = 1.0;

            // Новый абсолютный зум
            double nz = oldZoom * std::pow(kZoomStep, steps);
            if (nz < kZoomMin) nz = kZoomMin;
            if (nz > kZoomMax) nz = kZoomMax;

            if (qFuzzyCompare(nz, oldZoom))
            {
                we->accept();
                return true;
            }

            const double factor = nz / oldZoom;
            const QPointF viewPos = ui->graphView->viewport()->mapFromGlobal(QCursor::pos());

            // Точка сцены под курсором до зума
            QTransform invBefore = ui->graphView->viewportTransform().inverted();
            const QPointF scenePosBefore = invBefore.map(viewPos);

            ui->graphView->setTransformationAnchor(QGraphicsView::NoAnchor);
            ui->graphView->scale(factor, factor);

            // Точка сцены под курсором после зума
            QTransform invAfter = ui->graphView->viewportTransform().inverted();
            const QPointF scenePosAfter = invAfter.map(viewPos);

            // Компенсируем разницу
            const QPointF sceneDelta = scenePosAfter - scenePosBefore;
            ui->graphView->translate(sceneDelta.x(), sceneDelta.y());

            // Возвращаем обычное поведение
            ui->graphView->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

            _m_zoom = nz;

            updateAllPointNumbers();
            ui->graphView->viewport()->update();

            if (Document::Ptr doc = currentDocument())
                saveCurrentViewState(doc);

            we->accept();
            return true;
        }
        case QEvent::ContextMenu:
        {
            if (_drawingRuler || _isDrawingRuler)
            {
                clearAllHandleHoverEffects();
                hideGhost();
                event->accept();
                return true;
            }
            QContextMenuEvent* ce = static_cast<QContextMenuEvent*>(event);

            Document::Ptr doc = currentDocument();
            const QPoint  viewPos = ce->pos();
            const QPointF scenePos = ui->graphView->mapToScene(viewPos);
            QGraphicsItem* item = ui->graphView->itemAt(viewPos);
            if (!item || item == doc->videoRect)
            {
                if (QGraphicsItem* byEdge = pickItemByEdgeAt(ui->graphView, viewPos))
                    item = byEdge;
            }

            if (DragCircle* circle = dynamic_cast<DragCircle*>(item))
            {
                if (QGraphicsItem* parent = circle->parentItem())
                {
                    if (qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(parent))
                    {
                        QMenu menu {ui->graphView};
                        QAction* actRemove = menu.addAction(u8"Удалить узел");
                        QAction* actResume = menu.addAction(u8"Возобновить редактирование");
                        QAction* actRecalc = menu.addAction(u8"Пересчитать нумерацию");
                        QAction* actClockwise = menu.addAction(u8"Нумерация по часовой стрелке");
                        QAction* actCounterClockwise = menu.addAction(u8"Нумерация против часовой стрелки");

                        QAction* chosen = menu.exec(ce->globalPos());

                        if (chosen == actRemove)
                        {
                            const QVector<qgraph::DragCircle*>& circles = polyline->circles();
                            const int pointIndex = circles.indexOf(circle);

                            if (pointIndex < 0)
                            {
                                ce->accept();
                                return true;
                            }

                            const QPointF point = circle->scenePos();

                            if (_currentHoveredHandle == circle)
                                _currentHoveredHandle = nullptr;

                            if (_lastHoverHandle == circle)
                                _lastHoverHandle = nullptr;

                            if (_ghostTarget == circle)
                            {
                                _ghostTarget = nullptr;

                                if (_ghostHandle)
                                    _ghostHandle->setVisible(false);
                            }

                            if (_m_dragHandle == circle)
                            {
                                _m_dragHandle = nullptr;
                                _m_isDraggingHandle = false;
                            }

                            if (polyline->removePointAtIndex(pointIndex))
                            {
                                if (QUndoStack* stack = activeUndoStack())
                                {
                                    undo::NodeEdit::Data data;
                                    data.type = undo::NodeEdit::Type::RemovePolylineNode;
                                    data.pointIndex = pointIndex;
                                    data.point = point;

                                    stack->push(new undo::NodeEdit(currentDocument().get(),
                                                                   polyline,
                                                                   data,
                                                                   u8"Удаление узла"));
                                }

                                if (Document::Ptr doc = currentDocument(); doc && !doc->isModified)
                                {
                                    doc->isModified = true;
                                    updateFileListDisplay(doc->filePath);
                                }
                            }
                        }
                        else if (chosen == actResume)
                        {
                            const QVector<qgraph::DragCircle*>& circles = polyline->circles();
                            const int resumeIndex = circles.indexOf(circle);

                            if (resumeIndex < 0)
                            {
                                ce->accept();
                                return true;
                            }

                            const QVector<QPointF> pointsBefore = polyline->points();
                            const QVector<QPointF> removedPoints = pointsBefore.mid(resumeIndex + 1);

                            _resumeEditing = true;

                            // Возобновление редактирования: выбранная точка остается,
                            // все точки после нее удаляются и будут нарисованы заново
                            polyline->setClosed(false, false);

                            for (int i = 0; i < removedPoints.size(); ++i)
                                polyline->removeLastPointForce(false);

                            Document::Ptr doc = currentDocument();
                            QUndoStack* stack = activeUndoStack();

                            if (doc && stack)
                            {
                                undo::ResumeEdit::Data data;
                                data.type = undo::ResumeEdit::Type::Polyline;
                                data.resumeIndex = resumeIndex;
                                data.removedPoints = removedPoints;
                                data.addedPoints.clear();

                                data.closedBefore = true;
                                data.closedAfter = false;

                                stack->push(new undo::ResumeEdit(doc.get(),
                                                                 polyline,
                                                                 data,
                                                                 u8"Возобновление полилинии"));

                                if (!doc->isModified)
                                {
                                    doc->isModified = true;
                                    updateFileListDisplay(doc->filePath);
                                }
                            }

                            _drawingPolyline = true;
                            _polyline = polyline;
                            setSceneItemsMovable(false);
                            updateModeLabel();

                            _polyline->setModificationCallback([this,
                                                                resumeIndex,
                                                                removedPoints]()
                            {
                                if (_drawingPolyline && _polyline && _polyline->isClosed())
                                {
                                    _drawingPolyline = false;
                                    updateModeLabel();

                                    _polyline->updatePointNumbers();
                                    apply_LineWidth_ToItem(_polyline);
                                    apply_PointSize_ToItem(_polyline);
                                    apply_NumberSize_ToItem(_polyline);

                                    Document::Ptr doc = currentDocument();
                                    QUndoStack* stack = activeUndoStack();

                                    if (doc && stack)
                                    {
                                        undo::ResumeEdit::Data data;
                                        data.type = undo::ResumeEdit::Type::Polyline;
                                        data.resumeIndex = resumeIndex;
                                        data.removedPoints.clear();
                                        data.addedPoints.clear();
                                        data.closedBefore = false;
                                        data.closedAfter = true;

                                        stack->push(new undo::ResumeEdit(doc.get(),
                                                                         _polyline,
                                                                         data,
                                                                         u8"Завершение продолжения полилинии"));

                                        if (!doc->isModified)
                                        {
                                            doc->isModified = true;
                                            updateFileListDisplay(doc->filePath);
                                        }
                                    }

                                    _polyline = nullptr;
                                    _resumeEditing = false;

                                    raiseAllHandlesToTop();
                                    setSceneItemsMovable(true);
                                }
                            });
                        }
                        else if (chosen == actRecalc)
                        {
                            Document::Ptr doc = currentDocument();
                            QUndoStack* stack = activeUndoStack();

                            if (!doc || !stack)
                            {
                                ce->accept();
                                return true;
                            }

                            const QVector<qgraph::DragCircle*>& circles = polyline->circles();
                            const int idx = circles.indexOf(circle);

                            if (idx < 0)
                            {
                                ce->accept();
                                return true;
                            }

                            const QVector<QPointF> pointsBefore = polyline->points();

                            if (pointsBefore.isEmpty())
                            {
                                ce->accept();
                                return true;
                            }

                            double areaBefore = 0.0;

                            for (int i = 0; i < pointsBefore.size(); ++i)
                            {
                                const QPointF& firstPoint = pointsBefore[i];
                                const QPointF& secondPoint = pointsBefore[(i + 1) % pointsBefore.size()];

                                areaBefore += firstPoint.x() * secondPoint.y()
                                            - secondPoint.x() * firstPoint.y();
                            }

                            const QPointF firstPointBefore = pointsBefore.first();
                            const bool clockwiseBefore = (areaBefore > 0.0);

                            for (int i = 0; i < idx; ++i)
                                polyline->rotatePointsClockwise();

                            const QVector<QPointF> pointsAfter = polyline->points();

                            if (pointsAfter.isEmpty())
                            {
                                ce->accept();
                                return true;
                            }

                            double areaAfter = 0.0;

                            for (int i = 0; i < pointsAfter.size(); ++i)
                            {
                                const QPointF& firstPoint = pointsAfter[i];
                                const QPointF& secondPoint = pointsAfter[(i + 1) % pointsAfter.size()];

                                areaAfter += firstPoint.x() * secondPoint.y()
                                           - secondPoint.x() * firstPoint.y();
                            }

                            const QPointF firstPointAfter = pointsAfter.first();
                            const bool clockwiseAfter = (areaAfter > 0.0);

                            if (firstPointBefore != firstPointAfter
                                || clockwiseBefore != clockwiseAfter)
                            {
                                undo::NumberingEdit::Data data;
                                data.type = undo::NumberingEdit::Type::Polyline;
                                data.polylineFirstPointBefore = firstPointBefore;
                                data.polylineFirstPointAfter = firstPointAfter;
                                data.polylineClockwiseBefore = clockwiseBefore;
                                data.polylineClockwiseAfter = clockwiseAfter;

                                stack->push(new undo::NumberingEdit(doc.get(),
                                                                    polyline,
                                                                    data,
                                                                    u8"Пересчет нумерации"));

                                if (!doc->isModified)
                                {
                                    doc->isModified = true;
                                    updateFileListDisplay(doc->filePath);
                                }

                                if (doc->scene)
                                    doc->scene->update();
                            }
                        }
                        else if (chosen == actClockwise)
                        {
                            Document::Ptr doc = currentDocument();
                            QUndoStack* stack = activeUndoStack();

                            if (!doc || !stack)
                            {
                                ce->accept();
                                return true;
                            }

                            const QVector<QPointF> pointsBefore = polyline->points();

                            if (pointsBefore.isEmpty())
                            {
                                ce->accept();
                                return true;
                            }

                            double areaBefore = 0.0;

                            for (int i = 0; i < pointsBefore.size(); ++i)
                            {
                                const QPointF& firstPoint = pointsBefore[i];
                                const QPointF& secondPoint = pointsBefore[(i + 1) % pointsBefore.size()];

                                areaBefore += firstPoint.x() * secondPoint.y()
                                            - secondPoint.x() * firstPoint.y();
                            }

                            const QPointF firstPointBefore = pointsBefore.first();
                            const bool clockwiseBefore = (areaBefore > 0.0);

                            polyline->renumberFromHandleClockwise(circle);

                            const QVector<QPointF> pointsAfter = polyline->points();

                            if (pointsAfter.isEmpty())
                            {
                                ce->accept();
                                return true;
                            }

                            double areaAfter = 0.0;

                            for (int i = 0; i < pointsAfter.size(); ++i)
                            {
                                const QPointF& firstPoint = pointsAfter[i];
                                const QPointF& secondPoint = pointsAfter[(i + 1) % pointsAfter.size()];

                                areaAfter += firstPoint.x() * secondPoint.y()
                                           - secondPoint.x() * firstPoint.y();
                            }

                            const QPointF firstPointAfter = pointsAfter.first();
                            const bool clockwiseAfter = (areaAfter > 0.0);

                            if (firstPointBefore != firstPointAfter
                                || clockwiseBefore != clockwiseAfter)
                            {
                                undo::NumberingEdit::Data data;
                                data.type = undo::NumberingEdit::Type::Polyline;
                                data.polylineFirstPointBefore = firstPointBefore;
                                data.polylineFirstPointAfter = firstPointAfter;
                                data.polylineClockwiseBefore = clockwiseBefore;
                                data.polylineClockwiseAfter = clockwiseAfter;

                                stack->push(new undo::NumberingEdit(doc.get(),
                                                                    polyline,
                                                                    data,
                                                                    u8"Нумерация по часовой стрелке"));

                                if (!doc->isModified)
                                {
                                    doc->isModified = true;
                                    updateFileListDisplay(doc->filePath);
                                }

                                if (doc->scene)
                                    doc->scene->update();
                            }
                        }
                        else if (chosen == actCounterClockwise)
                        {
                            Document::Ptr doc = currentDocument();
                            QUndoStack* stack = activeUndoStack();

                            if (!doc || !stack)
                            {
                               ce->accept();
                               return true;
                            }

                            const QVector<QPointF> pointsBefore = polyline->points();

                            if (pointsBefore.isEmpty())
                            {
                                ce->accept();
                                return true;
                            }

                            double areaBefore = 0.0;

                            for (int i = 0; i < pointsBefore.size(); ++i)
                            {
                                const QPointF& firstPoint = pointsBefore[i];
                                const QPointF& secondPoint = pointsBefore[(i + 1) % pointsBefore.size()];

                                areaBefore += firstPoint.x() * secondPoint.y()
                                            - secondPoint.x() * firstPoint.y();
                            }

                            const QPointF firstPointBefore = pointsBefore.first();
                            const bool clockwiseBefore = (areaBefore > 0.0);

                            polyline->renumberFromHandleCounterClockwise(circle);

                            const QVector<QPointF> pointsAfter = polyline->points();

                            if (pointsAfter.isEmpty())
                            {
                                ce->accept();
                                return true;
                            }

                            double areaAfter = 0.0;

                            for (int i = 0; i < pointsAfter.size(); ++i)
                            {
                                const QPointF& firstPoint = pointsAfter[i];
                                const QPointF& secondPoint = pointsAfter[(i + 1) % pointsAfter.size()];

                                areaAfter += firstPoint.x() * secondPoint.y()
                                          - secondPoint.x() * firstPoint.y();
                            }

                            const QPointF firstPointAfter = pointsAfter.first();
                            const bool clockwiseAfter = (areaAfter > 0.0);

                            if (firstPointBefore != firstPointAfter
                                || clockwiseBefore != clockwiseAfter)
                            {
                                undo::NumberingEdit::Data data;
                                data.type = undo::NumberingEdit::Type::Polyline;
                                data.polylineFirstPointBefore = firstPointBefore;
                                data.polylineFirstPointAfter = firstPointAfter;
                                data.polylineClockwiseBefore = clockwiseBefore;
                                data.polylineClockwiseAfter = clockwiseAfter;

                                stack->push(new undo::NumberingEdit(doc.get(),
                                                                    polyline,
                                                                    data,
                                                                    u8"Нумерация против часовой стрелки"));

                                if (!doc->isModified)
                                {
                                    doc->isModified = true;
                                    updateFileListDisplay(doc->filePath);
                                }

                                if (doc->scene)
                                    doc->scene->update();
                            }
                        }
                        ce->accept();
                        return true;
                    }
                    if (qgraph::Line* line = dynamic_cast<qgraph::Line*>(parent))
                    {
                        QMenu menu(ui->graphView);

                        QAction* actRemove = menu.addAction(u8"Удалить узел");
                        QAction* actResume = menu.addAction(u8"Возобновить редактирование");
                        QAction* actRecalc = nullptr;
                        const  QVector<qgraph::DragCircle*>& circles = line->circles();
                        const int idx = circles.indexOf(circle);
                        if ((idx == 0) || (idx == circles.size() - 1))
                            actRecalc = menu.addAction(u8"Пересчитать нумерацию");

                        QAction* chosen = menu.exec(ce->globalPos());

                        if (chosen == actRemove)
                        {
                            const QVector<qgraph::DragCircle*>& circles = line->circles();
                            const int pointIndex = circles.indexOf(circle);

                            if (pointIndex < 0)
                            {
                                ce->accept();
                                return true;
                            }

                            const QPointF point = circle->scenePos();

                            if (_currentHoveredHandle == circle)
                                _currentHoveredHandle = nullptr;

                            if (_lastHoverHandle == circle)
                                _lastHoverHandle = nullptr;

                            if (_ghostTarget == circle)
                            {
                                _ghostTarget = nullptr;

                                if (_ghostHandle)
                                    _ghostHandle->setVisible(false);
                            }

                            if (_m_dragHandle == circle)
                            {
                                _m_dragHandle = nullptr;
                                _m_isDraggingHandle = false;
                            }

                            if (line->removePointAtIndex(pointIndex))
                            {
                                if (QUndoStack* stack = activeUndoStack())
                                {
                                    undo::NodeEdit::Data data;
                                    data.type = undo::NodeEdit::Type::RemoveLineNode;
                                    data.pointIndex = pointIndex;
                                    data.point = point;

                                    stack->push(new undo::NodeEdit(currentDocument().get(),
                                                                   line,
                                                                   data,
                                                                   u8"Удаление узла"));
                                }

                                if (Document::Ptr doc = currentDocument(); doc && !doc->isModified)
                                {
                                    doc->isModified = true;
                                    updateFileListDisplay(doc->filePath);
                                }
                            }
                        }
                        else if (chosen == actResume)
                        {
                            const QVector<qgraph::DragCircle*>& circles = line->circles();
                            const int resumeIndex = circles.indexOf(circle);

                            if (resumeIndex < 0)
                            {
                                ce->accept();
                                return true;
                            }

                            const QVector<QPointF> pointsBefore = line->points();
                            const QVector<QPointF> removedPoints = pointsBefore.mid(resumeIndex + 1);

                            _resumeEditing = true;

                            line->setClosed(false, false);

                            for (int i = 0; i < removedPoints.size(); ++i)
                                line->removeLastPointForce(false);

                            Document::Ptr doc = currentDocument();
                            QUndoStack* stack = activeUndoStack();

                            if (doc && stack)
                            {
                                undo::ResumeEdit::Data data;
                                data.type = undo::ResumeEdit::Type::Line;
                                data.resumeIndex = resumeIndex;
                                data.removedPoints = removedPoints;
                                data.addedPoints.clear();
                                data.closedBefore = true;
                                data.closedAfter = false;

                                stack->push(new undo::ResumeEdit(doc.get(),
                                                                 line,
                                                                 data,
                                                                 u8"Возобновление линии"));

                                if (!doc->isModified)
                                {
                                    doc->isModified = true;
                                    updateFileListDisplay(doc->filePath);
                                }
                            }

                            line->updatePointNumbers();

                            _drawingLine = true;
                            _line = line;
                            updateModeLabel();

                            setSceneItemsMovable(false);

                            _line->setModificationCallback([this,
                                                            resumeIndex,
                                                            removedPoints]()
                            {
                                if (_drawingLine && _line && _line->isClosed())
                                {
                                    _drawingLine = false;
                                    updateModeLabel();

                                    _line->updatePointNumbers();
                                    apply_LineWidth_ToItem(_line);
                                    apply_PointSize_ToItem(_line);
                                    apply_NumberSize_ToItem(_line);

                                    Document::Ptr doc = currentDocument();
                                    QUndoStack* stack = activeUndoStack();

                                    if (doc && stack)
                                    {
                                        undo::ResumeEdit::Data data;
                                        data.type = undo::ResumeEdit::Type::Line;
                                        data.resumeIndex = resumeIndex;
                                        data.removedPoints.clear();
                                        data.addedPoints.clear();
                                        data.closedBefore = false;
                                        data.closedAfter = true;

                                        stack->push(new undo::ResumeEdit(doc.get(),
                                                                         _line,
                                                                         data,
                                                                         u8"Завершение продолжения линии"));

                                        if (!doc->isModified)
                                        {
                                            doc->isModified = true;
                                            updateFileListDisplay(doc->filePath);
                                        }
                                    }

                                    _line = nullptr;
                                    _resumeEditing = false;

                                    raiseAllHandlesToTop();
                                    setSceneItemsMovable(true);
                                }
                            });
                        }
                        else if (actRecalc && chosen == actRecalc)
                        {
                            Document::Ptr doc = currentDocument();
                            QUndoStack* stack = activeUndoStack();

                            if (!doc || !stack)
                            {
                                ce->accept();
                                return true;
                            }

                            const bool numberingBefore = line->isNumberingFromLast();
                            const bool numberingAfter = (idx == circles.size() - 1);

                            if (numberingBefore != numberingAfter)
                            {
                                line->setNumberingFromLast(numberingAfter);
                                line->updatePointNumbers();

                                undo::NumberingEdit::Data data;
                                data.type = undo::NumberingEdit::Type::Line;
                                data.lineNumberingFromLastBefore = numberingBefore;
                                data.lineNumberingFromLastAfter = numberingAfter;

                                stack->push(new undo::NumberingEdit(doc.get(),
                                                                    line,
                                                                    data,
                                                                    u8"Пересчет нумерации"));

                                if (!doc->isModified)
                                {
                                    doc->isModified = true;
                                    updateFileListDisplay(doc->filePath);
                                }

                                if (doc->scene)
                                    doc->scene->update();
                            }
                        }
                        ce->accept();
                        return true;
                    }
                    if (qgraph::Rectangle* rect = dynamic_cast<qgraph::Rectangle*>(parent))
                    {
                        QMenu menu(ui->graphView);
                        QAction* actRecalc = menu.addAction(u8"Пересчитать нумерацию");

                        QAction* chosen = menu.exec(ce->globalPos());
                        if (chosen == actRecalc)
                        {
                            Document::Ptr doc = currentDocument();
                            QUndoStack* stack = activeUndoStack();

                            if (!doc || !stack)
                            {
                                ce->accept();
                                return true;
                            }

                            const int numberingBefore = rect->numberingOffset();

                            rect->recalcNumberingFromHandle(circle);

                            const int numberingAfter = rect->numberingOffset();

                            if (numberingBefore != numberingAfter)
                            {
                                undo::NumberingEdit::Data data;
                                data.type = undo::NumberingEdit::Type::Rectangle;
                                data.rectangleNumberingOffsetBefore = numberingBefore;
                                data.rectangleNumberingOffsetAfter = numberingAfter;

                                stack->push(new undo::NumberingEdit(doc.get(),
                                                                    rect,
                                                                    data,
                                                                    u8"Пересчет нумерации"));
                                if (!doc->isModified)
                                {
                                    doc->isModified = true;
                                    updateFileListDisplay(doc->filePath);
                                }
                                if (doc->scene)
                                    doc->scene->update();
                            }
                        }
                        ce->accept();
                        return true;
                    }

                }
            }

            if (qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(item))
            {
                const QPainterPath p = polyline->path();

                QPainterPathStroker stroker;
                const qreal rScene = viewPixelsToSceneRadius(ui->graphView,
                                                            effectiveViewPickPx(_edgePickRadius));

                const QPointF lp0 = polyline->mapFromScene(scenePos);
                const QPointF lp1 = polyline->mapFromScene(scenePos + QPointF(rScene, 0));
                const qreal rLocal = QLineF(lp0, lp1).length();

                const qreal w = qMax<qreal>(2.0 * rLocal, polyline->pen().widthF() + 1.0);
                stroker.setWidth(w);

                const QPainterPath stroke = stroker.createStroke(p);
                const QPointF localPos = polyline->mapFromScene(scenePos);
                if (!stroke.contains(localPos))
                    break;

                QMenu menu(ui->graphView);
                QAction* actAdd = menu.addAction(u8"Добавить узел");
                QAction* actToLine = menu.addAction(u8"Полилиния -> линия");

                QAction* chosen = menu.exec(ce->globalPos());
                if (chosen == actAdd)
                {
                    int pointIndex = -1;
                    QPointF point;

                    if (polyline->findInsertPointAt(scenePos, &pointIndex, &point))
                    {
                        QUndoStack* stack = activeUndoStack();

                        if (stack && polyline->insertPointAtIndex(pointIndex, point))
                        {
                            undo::NodeEdit::Data data;
                            data.type = undo::NodeEdit::Type::InsertPolylineNode;
                            data.pointIndex = pointIndex;
                            data.point = point;

                            stack->push(new undo::NodeEdit(currentDocument().get(),
                                                           polyline,
                                                           data,
                                                           u8"Добавление узла"));

                            if (Document::Ptr doc = currentDocument(); doc && !doc->isModified)
                            {
                                doc->isModified = true;
                                updateFileListDisplay(doc->filePath);
                            }
                        }
                    }
                }
                else if (chosen == actToLine)
                {
                    Document::Ptr doc = currentDocument();
                    QUndoStack* stack = activeUndoStack();

                    if (!doc || !doc->scene || !stack)
                    {
                        ce->accept();
                        return true;
                    }
                    if (polyline->points().size() < 2)
                    {
                        ce->accept();
                        return true;
                    }
                    undo::Replace::Data data;
                    data.type = undo::Replace::Type::PolylineToLine;
                    data.scenePos = scenePos;

                    undo::Replace* command = new undo::Replace(doc.get(),
                                                               polyline,
                                                               data,
                                                               u8"Полилиния -> линия");
                    if (command->isValid())
                    {
                        stack->push(command);

                        if (!doc->isModified)
                        {
                            doc->isModified = true;
                            updateFileListDisplay(doc->filePath);
                        }
                    }
                    else
                    {
                        delete command;
                    }
                }
                ce->accept();
                return true;
            }
            if (qgraph::Line* line = dynamic_cast<qgraph::Line*>(item))
            {
                const QPainterPath p = line->path();

                QPainterPathStroker stroker;
                const qreal rScene = viewPixelsToSceneRadius(ui->graphView,
                                                            effectiveViewPickPx(_edgePickRadius));

                const QPointF lp0 = line->mapFromScene(scenePos);
                const QPointF lp1 = line->mapFromScene(scenePos + QPointF(rScene, 0));
                const qreal rLocal = QLineF(lp0, lp1).length();

                const qreal w = qMax<qreal>(2.0 * rLocal, line->pen().widthF() + 1.0);
                stroker.setWidth(w);

                const QPainterPath stroke = stroker.createStroke(p);
                const QPointF localPos = line->mapFromScene(scenePos);
                if (!stroke.contains(localPos))
                    break;

                QMenu menu(ui->graphView);
                QAction* actAdd = menu.addAction(u8"Добавить узел");
                QAction* actMerge = menu.addAction(u8"Объединить линии");
                QAction* actSplit = menu.addAction(u8"Разорвать линию");
                QAction* actToPolyline = menu.addAction(u8"Линия -> полилиния");

                QAction* chosen = menu.exec(ce->globalPos());
                if (chosen == actAdd)
                {
                    int pointIndex = -1;
                    QPointF point;

                    if (line->findInsertPointAt(scenePos, &pointIndex, &point))
                    {
                        QUndoStack* stack = activeUndoStack();

                        if (stack && line->insertPointAtIndex(pointIndex, point))
                        {
                            undo::NodeEdit::Data data;
                            data.type = undo::NodeEdit::Type::InsertLineNode;
                            data.pointIndex = pointIndex;
                            data.point = point;

                            stack->push(new undo::NodeEdit(currentDocument().get(),
                                                           line,
                                                           data,
                                                           u8"Добавление узла"));

                            if (Document::Ptr doc = currentDocument(); doc && !doc->isModified)
                            {
                                doc->isModified = true;
                                updateFileListDisplay(doc->filePath);
                            }
                        }
                    }
                }
                else if (chosen == actMerge)
                {
                    startMergeLinesMode();
                    ce->accept();
                    return true;
                }
                else if (chosen == actSplit)
                {
                    performSplitLineByEdge(line, scenePos);
                }
                else if (chosen == actToPolyline)
                {
                    Document::Ptr doc = currentDocument();
                    QUndoStack* stack = activeUndoStack();

                    if (!doc || !doc->scene || !stack)
                    {
                        ce->accept();
                        return true;
                    }

                    if (line->points().size() < 2)
                    {
                        ce->accept();
                        return true;
                    }

                    undo::Replace::Data data;
                    data.type = undo::Replace::Type::LineToPolyline;
                    data.scenePos = scenePos;

                    undo::Replace* command = new undo::Replace(doc.get(),
                                                               line,
                                                               data,
                                                               u8"Линия -> полилиния");
                    if (command->isValid())
                    {
                        stack->push(command);

                        if (!doc->isModified)
                        {
                            doc->isModified = true;
                            updateFileListDisplay(doc->filePath);
                        }
                    }
                    else
                    {
                        delete command;
                    }
                }
                ce->accept();
                return true;
            }
            break;
        }
        default:
            break;
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
    Document::Ptr doc = currentDocument();
    QGraphicsScene* scene = doc->scene;

    if (event->modifiers() & Qt::ControlModifier)
    {
        QTransform base = ui->graphView->transform();
        if (_m_zoom != 0.0)
        {
            QTransform inv; inv.scale(1.0 / _m_zoom, 1.0 / _m_zoom);
            base = base * inv;
        }

        const double kZoomStep = MainWindow::_kZoomStep;
        const double kZoomMin = MainWindow::_kMinZoom;
        const double kZoomMax = MainWindow::_kMaxZoom;

        const int key = event->key();

        if (key == Qt::Key_Plus || key == Qt::Key_Equal)
        {
            double nz = _m_zoom * kZoomStep;
            if (nz < kZoomMin) nz = kZoomMin;
            if (nz > kZoomMax) nz = kZoomMax;

            _m_zoom = nz;
            ui->graphView->setTransform(base);      // Вернуть базу
            ui->graphView->scale(_m_zoom, _m_zoom); // Применить абсолютный масштаб
            updateAllPointNumbers();
            ui->graphView->viewport()->update();
            event->accept();
            return;
        }
        else if (key == Qt::Key_Minus)
        {
            double nz = _m_zoom / kZoomStep;
            if (nz < kZoomMin) nz = kZoomMin;
            if (nz > kZoomMax) nz = kZoomMax;

            _m_zoom = nz;
            ui->graphView->setTransform(base);
            ui->graphView->scale(_m_zoom, _m_zoom);
            updateAllPointNumbers();
            ui->graphView->viewport()->update();
            event->accept();
            return;
        }
        else if (key == Qt::Key_0)
        {
            // Сброс к базе
            fitImageToView();
            return;
        }
    }
    if (event->key() == Qt::Key_Delete)
    {
        on_actDelete_triggered();
        event->accept();
        return;
    }
    else if (event->key() == Qt::Key_E || event->key() == Qt::Key_R)
    {
        event->accept();
        return;
    }
    else if (event->key() == Qt::Key_N)
    {
        if (_drawingPolyline && _polyline)
        {
            _polyline->togglePointNumbers();
            _polyline->updatePointNumbers();
            if (scene)
                scene->update();

            event->accept();
            return;
        }

        if (_drawingLine && _line)
        {
            _line->togglePointNumbers();
            _line->updatePointNumbers();
            if (scene)
                scene->update();

            event->accept();
            return;
        }
        event->accept();
        return;
    }
    else if (event->key() == Qt::Key_B)
    {
        on_actBack_triggered();
        event->accept();
        return;
    }
    else if (event->key() == Qt::Key_L && event->modifiers() == Qt::NoModifier)
    {
        on_actRuler_triggered();
        event->accept();
        return;
    }
    else if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_S)
    {
        on_actSave_triggered(true);
        event->accept();
        return;
    }
    else if (event->key() == Qt::Key_K && event->modifiers() == Qt::NoModifier)
    {
        toggleRightSplitter();
        event->accept();
        return;
    }
    if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_Q)
    {
        close();
        event->accept();
        return;
    }
    if (event->modifiers() == Qt::NoModifier)
    {
        if (event->key() == Qt::Key_D)
        {
            nextImage();
            event->accept();
            return;
        }
        if (event->key() == Qt::Key_A)
        {
            prevImage();
            event->accept();
            return;
        }
    }
    if (event->key() == Qt::Key_Escape)
    {
        cancelRulerMode();
        event->accept();
        return;
    }
    QMainWindow::keyPressEvent(event);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    // Всегда запоминаем текущую открытую папку как последнюю
    if (!_currentFolderPath.isEmpty())
    {
        _lastUsedFolder = _currentFolderPath;
        saveLastUsedFolder();
    }

    // Проверяем есть ли несохраненные изменения
    if (hasUnsavedChanges())
    {
        QList<Document::Ptr> unsavedDocs = getUnsavedDocuments();

        int result = showUnsavedChangesDialog(unsavedDocs);

        switch (result)
        {
            case QDialogButtonBox::SaveAll:
                // Сохранить все
                saveAllDocuments();
                saveGeometry();
                event->accept();
                break;
            case QDialogButtonBox::Discard:
                // Не сохранять
                saveGeometry();
                event->accept();
                break;

            case QDialogButtonBox::Cancel:
                // Отмена
                event->ignore();
                break;

            default:
                event->ignore();
                break;
        }
    }
    else
    {
        // Нет несохраненных изменений - просто сохраняем геометрию и закрываем
        saveGeometry();
        event->accept();
    }
}

void MainWindow::showEvent(QShowEvent* event)
{
    QMainWindow::showEvent(event);

    if (!_openLastFolderOnFirstShow)
        return;

    _openLastFolderOnFirstShow = false;

    if (!_lastUsedFolder.isEmpty() && QDir(_lastUsedFolder).exists())
    {
        setWorkingFolder(_lastUsedFolder);
    }
}

void MainWindow::on_actOpen_triggered(bool)
{
    // Сначала проверяем несохраненные изменения,
    // и только потом даем выбирать новую папку
    if (!_currentFolderPath.isEmpty() && hasUnsavedChanges())
    {
        QList<Document::Ptr> unsavedDocs = getUnsavedDocuments();
        int result = showUnsavedChangesDialog(unsavedDocs);

        switch (result)
        {
            case QDialogButtonBox::SaveAll:
                saveAllDocuments();
                break;

            case QDialogButtonBox::Discard:
                break;

            case QDialogButtonBox::Cancel:
                return;

            default:
                return;
        }
    }
    QString initialDir = _lastUsedFolder.isEmpty()
                             ? QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
                             : _lastUsedFolder;

    // Создаем диалог выбора директории
    QString folderPath = QFileDialog::getExistingDirectory(
        this,
        u8"Выберите рабочую папку с изображениями",
        initialDir,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
        );
    if (!folderPath.isEmpty())
    {
        setWorkingFolder(folderPath);
    }
}

void MainWindow::on_actClose_triggered(bool)
{
    close();
}

void MainWindow::on_actSave_triggered(bool)
{
    if (Document::Ptr doc = currentDocument())
        saveAnnotationToFile(doc);
}

void MainWindow::on_actExit_triggered(bool)
{
    close();
}

void MainWindow::on_btnRect_clicked(bool)
{
    _drawingRectangle= true;
    _isInDrawingMode = true;
    setSceneItemsMovable(false);
}

void MainWindow::on_btnPolyline_clicked(bool)
{
    _drawingPolyline = true;
    _isInDrawingMode = true;
    setSceneItemsMovable(false);
}

void MainWindow::on_btnCircle_clicked(bool)
{
    _drawingCircle = true;
    _isInDrawingMode = true;
    setSceneItemsMovable(false);
}

void MainWindow::fitImageToView()
{
    Document::Ptr doc = currentDocument();

    if (!doc || !doc->scene || !doc->videoRect)
        return;

    QGraphicsScene* scene = doc->scene;
    qgraph::VideoRect* videoRect = doc->videoRect;

    scene->setSceneRect(videoRect->boundingRect().translated(videoRect->pos()));

    ui->graphView->resetTransform();
    ui->graphView->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
    ui->graphView->centerOn(scene->sceneRect().center());
    ui->graphView->horizontalScrollBar()->setValue(0);
    ui->graphView->verticalScrollBar()->setValue(0);

    if (_m_zoom <= 0.000001)
        _m_zoom = 1.0;

    const qreal zoom = ui->graphView->transform().m11();

    doc->viewState = {0, 0, zoom, scene->sceneRect().center()};
    doc->viewState.zoom = zoom;

    _m_zoom = zoom;

    updateAllPointNumbers();
    ui->graphView->viewport()->update();
}

void MainWindow::fileList_ItemChanged(QListWidgetItem* current, QListWidgetItem* previous)
{
    if (!current) return;

    restoreTemporaryRaisedZValues();

    // Полный сброс состояний ручек/призрака перед сменой сцены
    clearAllHandleHoverEffects();
    hideGhost();

    _m_isDraggingHandle = false;
    _m_dragHandle = nullptr;
    _handleDragging = false;
    _handleEditedItem = nullptr;
    _handleDragHadChanges = false;

    _currentHoveredHandle = nullptr;
    _ghostTarget = nullptr;

    // Сохраняем предыдущее состояние
    if (previous)
    {
        QVariant prevData = previous->data(Qt::UserRole);
        if (prevData.canConvert<Document::Ptr>())
        {
            Document::Ptr prevDoc = prevData.value<Document::Ptr>();

            // Сохраняем состояние просмотра
            saveCurrentViewState(prevDoc);

            // Отключаем сигнал changed для предыдущей сцены
            if (prevDoc->scene)
            {
                disconnect(prevDoc->scene, &QGraphicsScene::changed,
                           this, &MainWindow::onSceneChanged);
            }
        }
    }

    // Получаем текущий документ
    QVariant data = current->data(Qt::UserRole);

    if (!data.canConvert<Document::Ptr>())
        return;

    Document::Ptr doc = data.value<Document::Ptr>();

    if (!doc || !doc->scene)
        return;

    _currentImagePath = doc->filePath;

    ui->graphView->setScene(doc->scene);

    setPolygonListModelForCurrentDocument();

    connect(doc->scene,
            &QGraphicsScene::selectionChanged,
            this,
            &MainWindow::onSceneSelectionChanged,
            Qt::UniqueConnection);

    connect(doc->scene,
            &QGraphicsScene::changed,
            this,
            &MainWindow::onSceneChanged,
            Qt::UniqueConnection);

    doc->scene->installEventFilter(this);
    doc->scene->setProperty("classChangeReceiver", QVariant::fromValue(static_cast<QObject*>(this)));
    doc->scene->setProperty("shapeDeleteReceiver", QVariant::fromValue(static_cast<QObject*>(this)));
    doc->scene->setProperty("fillShapeWhenSelected", _vstyle.fillShapeWhenSelected);

    ensureGhostAllocated();

    if (doc->videoRect && !doc->videoRect->pixmap().isNull())
        updateImageSizeLabel(doc->videoRect->pixmap().size());
    else
        updateImageSizeLabel(QSize());

    restoreViewState(doc);
    updatePolygonListForCurrentScene();
    updateCoordinateList();
    updateWindowTitle();
    raiseAllHandlesToTop();

    if (_undoGroup && doc->_undoStack)
    {
        _undoGroup->setActiveStack(doc->_undoStack.get());
        _undoView->setStack(doc->_undoStack.get());
    }
}

void MainWindow::onPolylineModified()
{
    // Любое изменение полилинии помечает документ как измененный
    Document::Ptr doc = currentDocument();
    if (doc && !doc->isModified)
    {
        doc->isModified = true;
        updateFileListDisplay(doc->filePath);
    }
}

void MainWindow::onSceneChanged()
{
    // Игнорируем изменения во время загрузки
    if (_loadingNow)
        return;

    // Во время перетаскивания фигур не пересчитываем координаты
    if (_moveInProgress)
        return;

    // Обновляем координаты для выделенной фигуры
    if (_leftMouseButtonDown)
    {
        updateCoordinateList();
    }
}

void MainWindow::onPolygonListSelectionChanged()
{
    if (_syncingSelection)
        return;

    QItemSelectionModel* selectionModel = ui->polygonList->selectionModel();

    if (!selectionModel)
        return;

    const QModelIndexList selectedRows = selectionModel->selectedRows();

    _syncingSelection = true;

    Document::Ptr doc = currentDocument();
    QGraphicsScene* scene = doc->scene;
    qgraph::VideoRect* videoRect = doc->videoRect;

    {
        QSignalBlocker blocker {scene}; (void) blocker;

        for (QGraphicsItem* item : scene->items())
        {
            if (!item)
                continue;

            if (item == videoRect
                || item == _tempRectItem
                || item == _tempCircleItem
                || item == _tempPolyline)
            {
                continue;
            }

            if (item->parentItem() != nullptr)
                continue;

            item->setSelected(false);
        }

        for (const QModelIndex& index : selectedRows)
        {
            QGraphicsItem* sceneItem = sceneItemFromListIndex(index);

            if (sceneItem)
                sceneItem->setSelected(true);
        }
    }

    updateCoordinateList();

    if (!selectedRows.isEmpty())
    {
        QGraphicsItem* sceneItem = sceneItemFromListIndex(selectedRows.first());

        if (sceneItem)
            ui->graphView->ensureVisible(sceneItem);
    }

    raiseSelectedShapesTemporarily();
    updateShapeListButtons();

    _syncingSelection = false;
}

void MainWindow::selectAllShapes()
{
    Document::Ptr doc = currentDocument();
    QGraphicsScene* scene = doc->scene;
    qgraph::VideoRect* videoRect = doc->videoRect;

    _syncingSelection = true;

    {
        QSignalBlocker blockScene(scene);
        QSignalBlocker blockList(ui->polygonList);

        ui->polygonList->clearSelection();

        for (QGraphicsItem* item : scene->items())
        {
            if (!item)
                continue;

            if (item == videoRect
                || item == _tempRectItem
                || item == _tempCircleItem
                || item == _tempPolyline)
            {
                continue;
            }

            if (item->parentItem() != nullptr)
                continue;

            item->setSelected(true);
        }

        ui->polygonList->selectAll();
    }

    _syncingSelection = false;

    updateCoordinateList();
    raiseSelectedShapesTemporarily();
    scene->update();
}

void MainWindow::on_actRect_triggered()
{
    _btnRectFlag = true;

    _drawingRectangle = true;
    _drawingCircle = false;
    _drawingPolyline = false;
    _drawingLine = false;
    _drawingPoint = false;
    updateModeLabel();
}

void MainWindow::on_actCircle_triggered()
{
    _btnCircleFlag = true;

    _drawingCircle = true;
    _drawingPolyline = false;
    _drawingRectangle = false;
    _drawingLine = false;
    _drawingPoint = false;
    updateModeLabel();
}

void MainWindow::on_actPolyline_triggered()
{
    _btnPolylineFlag = true;

    _drawingPolyline = true;
    _drawingRectangle = false;
    _drawingCircle = false;
    _drawingLine = false;
    _drawingPoint = false;
    updateModeLabel();
}

void MainWindow::on_actPoint_triggered()
{
    _btnPointFlag = true;

    _drawingPoint = true;
    _drawingRectangle = false;
    _drawingCircle = false;
    _drawingPolyline = false;
    _drawingLine = false;
    updateModeLabel();
}

void MainWindow::on_actLine_triggered()
{
    _btnLineFlag = true;

    _drawingLine = true;
    _drawingPoint = false;
    _drawingRectangle = false;
    _drawingCircle = false;
    _drawingPolyline = false;
    updateModeLabel();
}

void MainWindow::on_actRuler_triggered()
{
    Document::Ptr doc = currentDocument();
    QGraphicsScene* scene = doc->scene;

    // Отключаем другие режимы рисования
    _drawingLine      = false;
    _drawingPoint     = false;
    _drawingRectangle = false;
    _drawingCircle    = false;
    _drawingPolyline  = false;

    // Включаем режим линейки
    _drawingRuler     = true;
    _isDrawingRuler   = false;
    _isInDrawingMode  = true;

    setSceneItemsMovable(false);
    clearAllHandleHoverEffects();
    hideGhost();

    // При включении инструмента очищаем старую линейку
    if (_rulerLine)
    {
        scene->removeItem(_rulerLine);
        delete _rulerLine;
        _rulerLine = nullptr;
    }
    if (_rulerText)
    {
        scene->removeItem(_rulerText);
        delete _rulerText;
        _rulerText = nullptr;
    }

    updateModeLabel();
}

void MainWindow::on_actClosePolyline_triggered()
{
    if (_drawingPolyline && _polyline)
    {
        if (!_polyline->isClosed())
            _polyline->closePolyline();

        return;
    }
    if (_drawingLine && _line)
    {
        if (!_line->isClosed())
            _line->closeLine();

        return;
    }
}

void MainWindow::wheelEvent(QWheelEvent* event)
{
    // Сохраняем состояние при изменении масштаба
    if (Document::Ptr doc = currentDocument())
    {
        saveCurrentViewState(doc);
    }
    QMainWindow::wheelEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    // При изменении размера сохраняем текущее состояние
    if (Document::Ptr doc = currentDocument())
    {
        saveCurrentViewState(doc);
        QMainWindow::resizeEvent(event);
        restoreViewState(doc);
    }
    QMainWindow::resizeEvent(event);
}

void MainWindow::onCheckBoxPolygonLabel(QAbstractButton* button)
{
    QCheckBox* checkBox = qobject_cast<QCheckBox*>(button);
    if (checkBox)
    {
        QString selectedText = checkBox->property("itemText").toString();
    }
}

void MainWindow::setWorkingFolder(const QString& folderPath)
{
    // Проверяем, существует ли выбранная папка
    if (!QDir(folderPath).exists())
    {
        messageBox(
            this,
            QMessageBox::Warning,
            u8"Выбранная папка не существует!"
        );
        return;
    }

    // Если выбрали ту же самую папку - ничего не делаем
    if (!_currentFolderPath.isEmpty() &&
        QDir::cleanPath(_currentFolderPath) == QDir::cleanPath(folderPath))
    {
        return;
    }

    _ghostTarget = nullptr;
    _ghostHandle = nullptr;
    _lastHoverHandle = nullptr;
    _ghostActive = false;

    // Отвязываем view от сцены старого документа до удаления документов
    ui->graphView->setScene(nullptr);

    if (_undoView)
        _undoView->setStack(nullptr);

    // Полностью забываем предыдущее состояние проекта,
    // чтобы старые документы не участвовали в последующих проверках
    if (_undoGroup)
    {
        for (QMap<QString, Document::Ptr>::iterator it = _documentsMap.begin();
             it != _documentsMap.end(); ++it)
        {
            if (it.value() && it.value()->_undoStack)
                _undoGroup->removeStack(it.value()->_undoStack.get());
        }
    }

    _documentsMap.clear();
    _currentImagePath.clear();
    _scrollStates.clear();

    ui->fileList->clear();
    ui->polygonList->setModel(nullptr);
    ui->coordinateList->clear();

    updateWindowTitle();

    _currentFolderPath = folderPath;

    QDir dir(folderPath);
    loadClassesFromFile(dir.filePath("classes.yaml"));

    loadFilesFromFolder(folderPath);

    // Открыть первое изображение из папки
    if (ui->fileList->count() > 0)
    {
        ui->fileList->setCurrentRow(0);
        ui->fileList->scrollToItem(ui->fileList->currentItem());
    }
}

void MainWindow::onSceneItemRemoved(QGraphicsItem* item)
{
    Document::Ptr doc = currentDocument();
    qgraph::VideoRect* videoRect = doc->videoRect;

    // Пропускаем временные элементы и само изображение
    if (item == videoRect || item == _tempRectItem ||
        item == _tempCircleItem || item == _tempPolyline)
    {
        return;
    }
    clearLinePolylineStateForDeletedItem(item);
    if (_resumeEditing)
    {
        const qulonglong uid = item ? item->data(_roleUid).toULongLong() : 0;
        if (uid && uid == _resumeUid)
        {
            _resumeEditing = false;
            _resumeUid = 0;
            _drawingLine = false;
            _drawingPolyline = false;
            _line = nullptr;
            _polyline = nullptr;
            setSceneItemsMovable(true);
            updateModeLabel();
        }
    }

    if (item == _line)
    {
        _line = nullptr;
        _drawingLine = false;
        updateModeLabel();
    }
    if (item == _polyline)
    {
        _polyline = nullptr;
        _drawingPolyline = false;
        updateModeLabel();
    }

    removeListEntryBySceneItem(item);
}

void MainWindow::on_actDelete_triggered()
{
    Document::Ptr doc = currentDocument();
    if (!doc)
        return;

    QSet<QGraphicsItem*> shapes;

    const QList<QGraphicsItem*> selectedSceneItems = doc->scene->selectedItems();

    for (QGraphicsItem* item : selectedSceneItems)
    {
        if (!item)
            continue;

        QGraphicsItem* shape = item;

        if (qgraph::DragCircle* handle = dynamic_cast<qgraph::DragCircle*>(shape))
            shape = handle->parentItem();

        if (!shape)
            continue;

        if (shape == doc->videoRect)
            continue;

        // Удаляем только фигуры, которые есть в правом списке
        if (polygonListRowByItem(shape) < 0)
            continue;

        shapes.insert(shape);
    }

    // Собираем фигуры, выбранные в QListView
    if (ui && ui->polygonList && ui->polygonList->selectionModel())
    {
        QItemSelectionModel* selectionModel = ui->polygonList->selectionModel();
        const QModelIndexList selectedRows = selectionModel->selectedRows();

        for (const QModelIndex& index : selectedRows)
        {
            QGraphicsItem* shape = sceneItemFromListIndex(index);

            if (!shape)
                continue;

            if (shape == doc->videoRect)
                continue;

            shapes.insert(shape);
        }
    }

    if (shapes.isEmpty())
        return;

    if (shapes.size() > 1)
    {
        const QMessageBox::StandardButton answer = QMessageBox::question(
            this,
            QString::fromUtf8("Удаление фигур"),
            QString::fromUtf8("Удалить выбранные фигуры?"),
            QMessageBox::Yes | QMessageBox::No
        );

        if (answer != QMessageBox::Yes)
            return;
    }

    for (QGraphicsItem* shape : shapes)
    {
        // Если удаляемая фигура сейчас находится в режиме редактирования
        clearLinePolylineStateForDeletedItem(shape);
        shape->setSelected(false);
    }

    const QString description = (shapes.size() == 1)
        ? QString::fromUtf8("Удаление фигуры")
        : QString::fromUtf8("Удаление фигур");

    doc->_undoStack->push(new undo::Delete(doc, shapes, description));

    updateShapeListButtons();
    updateFileListDisplay(doc->filePath);

    if (doc->scene)
        doc->scene->update();
}

void MainWindow::on_actSettingsApp_triggered()
{
    Document::Ptr doc = currentDocument();
    QGraphicsScene* scene = doc->scene;

    MainWindow::VisualStyle visualStyleBackup = _vstyle;
    bool wasApplied = false;

    Settings settings {this};

    settings.loadGeometry();

    Settings::Values init;
    init.lineWidth         = _vstyle.lineWidth;
    init.handleSize        = _vstyle.handleSize;
    init.numberFontPt      = _vstyle.numberFontPt;
    init.pointSize         = _vstyle.pointSize;
    init.showNumbers = _vstyle.showNumbers;
    init.showSelectionFrame = _vstyle.showSelectionFrame;
    init.fillShapeWhenSelected = _vstyle.fillShapeWhenSelected;

    init.nodeColor         = _vstyle.handleColor;           // «Цвет узла»
    init.nodeSelectedColor = _vstyle.selectedHandleColor;   // «Цвет выбранного узла»
    init.numberColor       = _vstyle.numberColor;           // «Цвет нумерации»
    init.numberBgColor     = _vstyle.numberBgColor;         // «Фон нумерации»
    init.rectLineColor     = _vstyle.rectangleLineColor;    // «Линии прямоугольника»
    init.circleLineColor   = _vstyle.circleLineColor;       // «Линии окружности»
    init.polylineLineColor = _vstyle.polylineLineColor;     // «Линии полилинии»
    init.lineLineColor     = _vstyle.lineLineColor;
    init.pointColor        = _vstyle.pointColor;
    init.rulerColor        = _vstyle.rulerColor;

    init.closePolyline = _polylineCloseMode;    // Текущий режим замыкания
    init.finishLine    = _lineFinishMode;       // Текущий режим завершения рисования линии
    init.labelFontPt   = _vstyle.labelFontPt;   // Размер и шрифт
    init.labelFont     = _vstyle.labelFont;

    init.handlePickRadius = _ghostPickRadius;
    init.edgePickRadius = _edgePickRadius;

    config::base().getValue("view.keep_image_scale_per_image", init.keepImageScale);
    config::base().getValue("ui.keep_menu_visibility", init.keepMenuBarVisibility);

    settings.setValues(init);

    connect(&settings, &Settings::settingsApplied, this,
            [this, &visualStyleBackup, &wasApplied](const Settings::Values& v)
    {
        wasApplied = true;
        // 1) Обновляем визуальные настройки из v
        _vstyle.lineWidth = v.lineWidth;
        _vstyle.handleSize = v.handleSize;
        _ghostPickRadius = v.handlePickRadius;
        _edgePickRadius  = v.edgePickRadius;
        _vstyle.numberFontPt = v.numberFontPt;
        _vstyle.showNumbers = v.showNumbers;
        _vstyle.showSelectionFrame = v.showSelectionFrame;
        _vstyle.fillShapeWhenSelected = v.fillShapeWhenSelected;
        _vstyle.pointSize = v.pointSize;

        forEachScene([this](QGraphicsScene* sc)
        {
            if (!sc)
                return;

            sc->setProperty("fillShapeWhenSelected", _vstyle.fillShapeWhenSelected);
        });

        _vstyle.handleColor = v.nodeColor;
        _vstyle.selectedHandleColor = v.nodeSelectedColor;
        _vstyle.numberColor = v.numberColor;
        _vstyle.numberBgColor = v.numberBgColor;
        _vstyle.rectangleLineColor = v.rectLineColor;
        _vstyle.circleLineColor = v.circleLineColor;
        _vstyle.polylineLineColor = v.polylineLineColor;
        _vstyle.lineLineColor = v.lineLineColor;
        _vstyle.pointColor = v.pointColor;
        _vstyle.pointOutlineWidth = v.pointOutlineWidth;
        _vstyle.labelFontPt = v.labelFontPt;
        _vstyle.labelFont = v.labelFont;
        _vstyle.rulerColor = v.rulerColor;
        if (_rulerLine)
        {
            QPen pen = _rulerLine->pen();
            pen.setColor(_vstyle.rulerColor);
            _rulerLine->setPen(pen);
        }
        if (_rulerText)
        {
            _rulerText->setDefaultTextColor(_vstyle.rulerColor);
        }
        Document::Ptr doc = currentDocument();
        if (doc->scene)
            doc->scene->update();

        applyLabelFontToUi();

        // 2) Сохраняем и применяем
        saveVisualStyle();
        applyStyle_AllDocuments();

        forEachScene([this](QGraphicsScene* sc)
        {
            if (!sc)
                return;

            for (QGraphicsItem* item : sc->items())
            {
                if (!item)
                    continue;

                Document::Ptr doc = currentDocument();
                if (item == doc->videoRect
                    || item == _tempRectItem
                    || item == _tempCircleItem
                    || item == _tempPolyline)
                {
                    continue;
                }

                if (item->parentItem() != nullptr)
                    continue;

                const QString cls = item->data(0).toString();
                if (!cls.isEmpty())
                    applyClassColorToItem(item, cls);
            }

            sc->update();
        });

        if (doc->scene)
            onSceneSelectionChanged();

        // 3) Режим замыкания полилинии
        _polylineCloseMode = v.closePolyline;
        applyClosePolyline();

        config::base().setValue("polyline.close_mode",
                                static_cast<int>(_polylineCloseMode));

        _lineFinishMode = v.finishLine;
        applyFinishLine();

        config::base().setValue("line.finish_mode",
                                static_cast<int>(_lineFinishMode));

        // Масштаб при смене снимков
        config::base().setValue("view.keep_image_scale_per_image", v.keepImageScale);
        config::base().setValue("ui.keep_menu_visibility", v.keepMenuBarVisibility);
        config::base().saveFile();
        visualStyleBackup = _vstyle;

        forEachScene([this](QGraphicsScene* sc)
        {
            if (!sc)
                return;

            sc->setProperty("fillShapeWhenSelected", _vstyle.fillShapeWhenSelected);
        });
    });

    int res = settings.exec();
    settings.saveGeometry();

    if (res == QDialog::Rejected)
    {
        _vstyle = visualStyleBackup;
        saveVisualStyle();
        apply_LineWidth_ToScene(nullptr);
        apply_PointSize_ToScene(nullptr);
        apply_NumberSize_ToScene(nullptr);

        applyLabelFontToUi();

        applyClosePolyline();
        applyFinishLine();

        // Откат конфига
        saveVisualStyle();
        config::base().setValue("polyline.close_mode", static_cast<int>(_polylineCloseMode));
        config::base().setValue("line.finish_mode", static_cast<int>(_lineFinishMode));
        config::base().saveFile();

        applyStyle_AllDocuments();
        forEachScene([this](QGraphicsScene* sc)
        {
            if (!sc)
                return;
            Document::Ptr doc = currentDocument();

            for (QGraphicsItem* item : sc->items())
            {
                if (!item)
                    continue;

                if (item == doc->videoRect
                    || item == _tempRectItem
                    || item == _tempCircleItem
                    || item == _tempPolyline)
                {
                    continue;
                }

                if (item->parentItem() != nullptr)
                    continue;

                QString cls = item->data(0).toString();
                if (!cls.isEmpty())
                    applyClassColorToItem(item, cls);
            }

            sc->update();
        });

        if (scene)
            onSceneSelectionChanged();
    }
}

void MainWindow::on_actSettingsProj_triggered()
{
    _projPropsDialog->setProjectClasses(_projectClasses);
    _projPropsDialog->setProjectClassColors(_projectClassColors);

    QVector<int> geom{ -1, -1, 520, 360 };
    config::base().getValue("windows.project_settings.geometry", geom);
    if (geom.size() == 4) {
        if (geom[2] < 320) geom[2] = 320;
        if (geom[3] < 200) geom[3] = 200;
        if (geom[0] >= 0 && geom[1] >= 0)
            _projPropsDialog->move(geom[0], geom[1]);
        _projPropsDialog->resize(geom[2], geom[3]);
    }

    const int rc = _projPropsDialog->exec();

    const QRect r = (_projPropsDialog->isMaximized() || _projPropsDialog->isFullScreen())
                    ? _projPropsDialog->normalGeometry()
                    : _projPropsDialog->geometry();
    QVector<int> out { r.x(), r.y(), r.width(), r.height() };
    config::base().setValue("windows.project_settings.geometry", out);
    config::base().saveFile();

    if (rc == QDialog::Accepted)
    {
        _projectClasses = _projPropsDialog->projectClasses();
        _projectClassColors = _projPropsDialog->projectClassColors();
        saveProjectClasses(_projectClasses, _projectClassColors);
    }
}

void MainWindow::nextImage()
{
    if (!ui || !ui->fileList) return;

    QListWidget* list = ui->fileList;
    const int n = list->count();
    if (n <= 0) return;

    int row = list->currentRow();
    if (row < 0)
        row = 0;

    const int newRow = (row + 1) % n; // Зацикливание вперед
    if (newRow == row && n == 1)
        return; // Единственный файл - ничего не делаем

    list->setCurrentRow(newRow);
}

void MainWindow::prevImage()
{
    if (!ui || !ui->fileList) return;

    QListWidget* list = ui->fileList;
    const int n = list->count();
    if (n <= 0) return;

    int row = list->currentRow();
    if (row < 0)
        row = 0;

    const int newRow = (row - 1 + n) % n; // Зацикливание назад
    if (newRow == row && n == 1)
        return;

    list->setCurrentRow(newRow);
}

void MainWindow::on_actBack_triggered()
{
    moveSelectedItemsToBack();
}

void MainWindow::on_actViewNum_triggered()
{
    Document::Ptr doc = currentDocument();
    QGraphicsScene* scene = doc->scene;

    QList<QGraphicsItem*> selectedItems = scene->selectedItems();
    for (QGraphicsItem* item : selectedItems)
    {
        if (qgraph::Rectangle* rectangle = dynamic_cast<qgraph::Rectangle*>(item))
        {
            rectangle->togglePointNumbers();
        }
        else if (qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(item))
        {
            polyline->togglePointNumbers();
        }
        else if (qgraph::Line* line = dynamic_cast<qgraph::Line*>(item))
        {
            line->togglePointNumbers();
        }
    }
}

void MainWindow::on_actRotatePointsClockwise_triggered()
{
    Document::Ptr doc = currentDocument();
    QGraphicsScene* scene = doc->scene;

    QList<QGraphicsItem*> selectedItems = scene->selectedItems();
    for (QGraphicsItem* item : selectedItems)
    {
        if (qgraph::Rectangle* rectangle = dynamic_cast<qgraph::Rectangle*>(item))
        {
            rectangle->rotatePointsClockwise();
            updateAllPointNumbers();
        }
        else if (qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(item))
        {
            polyline->rotatePointsClockwise();
            updateAllPointNumbers();
        }
        else if (qgraph::Line* line = dynamic_cast<qgraph::Line*>(item))
        {
            line->rotatePointsClockwise();
            updateAllPointNumbers();
        }
    }
}

void MainWindow::on_actRotatePointsCounterClockwise_triggered()
{
    Document::Ptr doc = currentDocument();
    QGraphicsScene* scene = doc->scene;

    QList<QGraphicsItem*> selectedItems = scene->selectedItems();
    for (QGraphicsItem* item : selectedItems)
    {
        if (qgraph::Rectangle* rectangle = dynamic_cast<qgraph::Rectangle*>(item))
        {
            rectangle->rotatePointsCounterClockwise();
            updateAllPointNumbers();
        }
        else if (qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(item))
        {
            polyline->rotatePointsCounterClockwise();
            updateAllPointNumbers();
        }
        else if (qgraph::Line* line = dynamic_cast<qgraph::Line*>(item))
        {
            line->rotatePointsCounterClockwise();
            updateAllPointNumbers();
        }
    }
}

void MainWindow::on_Copy_triggered()
{
    copySelectedShapes();
}

void MainWindow::on_Paste_triggered()
{
    pasteCopiedShapesToCurrentScene();
}

void MainWindow::on_toggleSelectionFrame_triggered()
{
    Document::Ptr doc = currentDocument();
    QGraphicsScene* scene = doc->scene;

    QList<QGraphicsItem*> selectedItems = scene->selectedItems();
    for (QGraphicsItem* item : selectedItems)
    {
        if (qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(item))
        {
            polyline->toggleSelectionRect();
        }
        else if (qgraph::Line* line = dynamic_cast<qgraph::Line*>(item))
        {
            line->toggleSelectionRect();
        }
        else if (qgraph::Circle* circle = dynamic_cast<qgraph::Circle*>(item))
        {
            circle->toggleSelectionRect();
        }
    }
}

void MainWindow::on_actUndo_triggered()
{
    if (!_undoGroup)
        return;

    restoreTemporaryRaisedZValues();

    if (_undoGroup->canUndo())
        _undoGroup->undo();
}

void MainWindow::on_actRedo_triggered()
{
    if (!_undoGroup)
        return;

    restoreTemporaryRaisedZValues();

    if (_undoGroup->canRedo())
        _undoGroup->redo();
}

void MainWindow::on_actResetAnnotation_triggered()
{
    Document::Ptr doc = currentDocument();
    if (!doc)
        return;

    // Если разметки нет
    if (doc->polygonList.items.isEmpty())
        return;

    const QMessageBox::StandardButton answer = messageBox(
        this,
        QMessageBox::Question,
        u8"Удалить всю разметку на текущем снимке?",
        0,
        [](QMessageBox* box)
        {
            box->setWindowTitle(u8"Сбросить разметку");
            box->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            box->setDefaultButton(QMessageBox::No);

            if (QAbstractButton* yesButton = box->button(QMessageBox::Yes))
                yesButton->setText(u8"Да");

            if (QAbstractButton* noButton = box->button(QMessageBox::No))
                noButton->setText(u8"Нет");
        }
    );

    if (answer != QMessageBox::Yes)
        return;

    QVector<QGraphicsItem*> shapes = doc->polygonList.items;

    for (QGraphicsItem* shape : shapes)
    {
        if (!shape)
            continue;

        clearLinePolylineStateForDeletedItem(shape);
        shape->setSelected(false);
    }

    QUndoStack* stack = activeUndoStack();

    if (!stack)
        return;

    stack->push(new undo::ResetAnnotation(doc.get(),
                                          shapes,
                                          u8"Сброс разметки"));

    doc->isModified = true;
    updateFileListDisplay(doc->filePath);
    updateShapeListButtons();

    if (doc->scene)
        doc->scene->update();
}

void MainWindow::on_actRestoreAnnotation_triggered()
{
    Document::Ptr doc = currentDocument();
    if (!doc)
        return;

    const QString yamlPath = annotationPathFor(doc->filePath);

    if (!QFileInfo::exists(yamlPath))
    {
        messageBox(
            this,
            QMessageBox::Information,
            QString(u8"Файл разметки не найден:\n%1").arg(yamlPath)
        );
        return;
    }

    // Восстановление откатит все несохраненные изменения
    const bool hasUnsaved = doc->isModified || (doc->_undoStack && !doc->_undoStack->isClean());

    if (hasUnsaved || !doc->polygonList.items.isEmpty())
    {
        const QMessageBox::StandardButton answer = messageBox(
            this,
            QMessageBox::Question,
            u8"Восстановить разметку из файла .yaml?\nВсе несохранённые изменения будут потеряны",
            0,
            [](QMessageBox* box)
            {
                box->setWindowTitle(u8"Восстановить разметку");
                box->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
                box->setDefaultButton(QMessageBox::No);

                if (QAbstractButton* yesButton = box->button(QMessageBox::Yes))
                    yesButton->setText(u8"Да");

                if (QAbstractButton* noButton = box->button(QMessageBox::No))
                    noButton->setText(u8"Нет");
            }
        );

        if (answer != QMessageBox::Yes)
            return;
    }

    // Очищаем undo/redo стек
    if (doc->_undoStack)
    {
        doc->_undoStack->clear();
        doc->_undoStack->setClean();
    }

    // Помечаем документ неизмененным
    doc->isModified = false;
    updateFileListDisplay(doc->filePath);

    hideGhost();
    // Загружаем заново из yaml
    loadAnnotationFromFile(doc);

    doc->isModified = false;
    if (doc->_undoStack)
        doc->_undoStack->setClean();

    updateFileListDisplay(doc->filePath);
}

void MainWindow::on_actMenu_triggered()
{
    toggleMenuBarVisible();
}

void MainWindow::on_actFitImageToView_triggered()
{
    fitImageToView();
}

void MainWindow::on_actScrollBars_triggered(bool checked)
{
    ui->graphView->setHorizontalScrollBarPolicy(
        checked ? Qt::ScrollBarAlwaysOn : Qt::ScrollBarAlwaysOff);
    ui->graphView->setVerticalScrollBarPolicy(
        checked ? Qt::ScrollBarAlwaysOn : Qt::ScrollBarAlwaysOff);

    if (checked)
    {
        ui->graphView->setStyleSheet(
            "QScrollBar:vertical { width: 14px; }"
            "QScrollBar:horizontal { height: 14px; }"
            "QScrollBar::add-line, QScrollBar::sub-line { width: 14px; height: 14px; }"
            "QScrollBar::handle:vertical { min-height: 20px; }"
            "QScrollBar::handle:horizontal { min-width: 20px; }"
        );

        ui->graphView->horizontalScrollBar()->show();
        ui->graphView->verticalScrollBar()->show();
    }
    else
    {
        ui->graphView->setStyleSheet(QString());
        ui->graphView->horizontalScrollBar()->hide();
        ui->graphView->verticalScrollBar()->hide();
    }

    ui->graphView->updateGeometry();
    ui->graphView->viewport()->update();
}

void MainWindow::loadFilesFromFolder(const QString& folderPath)
{
    QDir directory(folderPath);
    _currentFolderPath = folderPath; // Сохраняем путь

    // Путь к папке в заголовке окна
    updateWindowTitle();

    ui->fileList->clear(); // Очищаем список перед добавлением
    // Путь к папке над полем FileList
    ui->listPath->setText(QDir::toNativeSeparators(folderPath));


    QStringList filters;
    filters << "*.jpg";
    filters << "*.jpeg";
    filters << "*.png";
    filters << "*.tif";
    filters << "*.tiff";
    QStringList files = directory.entryList(filters, QDir::Files);

    // Загружаем иконки из ресурсов
    QIcon modifiedIcon(":/images/resources/ok.svg");
    QIcon defaultIcon(":/images/resources/not_ok.svg");

    // Добавляем файлы
    for (const QString& filename : files)
    {
        QString filePath = directory.filePath(filename);
        QListWidgetItem* item = new QListWidgetItem(filename);
        bool hasValidAnnotation = false;
        if (hasAnnotationFile(filePath))
        {
            // Проверяем, не пустой ли YAML файл
            QString yamlPath = getAnnotationFilePath(filePath);
            if (!isYamlFileEmpty(yamlPath))
            {
                hasValidAnnotation = true;
            }
        }

        if (hasValidAnnotation)
        {
            item->setIcon(modifiedIcon);
        }
        else
        {
            item->setIcon(defaultIcon);
            //item->setText("* " + filename);
        }
        // Создаем документ и сохраняем его в данных элемента
        Document::Ptr doc = Document::create(filePath);
        item->setData(Qt::UserRole, QVariant::fromValue(doc));
        _documentsMap.insert(doc->filePath, doc);

        // Инициализируем индивидуальный стек для документа и регистрируем его в группе
        doc->_undoStack = std::make_unique<QUndoStack>();      // без родителя, владеем unique_ptr
        _undoGroup->addStack(doc->_undoStack.get());           // группа будет переключать активный

        QUndoStack* stack = doc->_undoStack.get();
        QObject::connect(stack, &QUndoStack::cleanChanged,
                         this, [this, stack](bool clean){
            if (_loadingNow)
                return;

            if (_undoGroup && _undoGroup->activeStack() != stack)
                return;

            if (Document::Ptr doc = currentDocument())
            {
                doc->isModified = !clean;
                updateFileListDisplay(doc->filePath);
            }
        });

        QObject::connect(stack, &QUndoStack::indexChanged,
                         this, [this, stack](int)
        {
            if (_loadingNow)
                return;

            if (_undoGroup && _undoGroup->activeStack() != stack)
                return;

            Document::Ptr doc = currentDocument();

            if (!doc || !doc->scene)
                return;

            const QList<QGraphicsItem*> items = doc->scene->items();

            for (QGraphicsItem* item : items)
            {
                if (!item)
                    continue;

                if (item == doc->videoRect
                    || item == _tempRectItem
                    || item == _tempCircleItem
                    || item == _tempPolyline)
                {
                    continue;
                }

                if (item->parentItem() != nullptr)
                    continue;

                const QString className = item->data(0).toString();

                if (!className.isEmpty())
                    applyClassColorToItem(item, className);

                apply_PointSize_ToItem(item);

                if (qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(item))
                    polyline->updatePointNumbers();
                else if (qgraph::Line* line = dynamic_cast<qgraph::Line*>(item))
                    line->updatePointNumbers();
                else if (qgraph::Rectangle* rectangle = dynamic_cast<qgraph::Rectangle*>(item))
                    rectangle->updatePointNumbers();

               apply_NumberSize_ToItem(item);
            }

            refreshShapeListOrderRole();
            updatePolygonListForCurrentScene();
            updateCoordinateList();

            restoreDrawingStateAfterStackChange();

            doc->scene->update();
        });

        ui->fileList->addItem(item);
        if (doc->loadImage())
        {
            loadAnnotationFromFile(doc, false);
        }
    }
}

void MainWindow::updatePolygonListForCurrentScene()
{
    Document::Ptr doc = currentDocument();

    if (!doc)
        return;

    doc->polygonList.items.clear();
    doc->polygonList.model.clear();
    doc->polygonList.model.setColumnCount(1);

    ui->coordinateList->clear();

    if (!doc->scene)
        return;

    QGraphicsScene* scene = doc->scene;
    qgraph::VideoRect* videoRect = doc->videoRect;

    QList<QGraphicsItem*> roots;
    for (QGraphicsItem* item : scene->items())
    {
        if (!item)
            continue;

        if (item == videoRect
            || item == _tempRectItem
            || item == _tempCircleItem
            || item == _tempPolyline)
        {
            continue;
        }

        if (item->parentItem() != nullptr)
            continue;

        // Открытая линия/полилиния после undo "Добавление линии/полилинии"
        // снова становится текущим объектом рисования и не должна попадать
        // в список завершенных фигур
        if (qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(item))
        {
            if (!polyline->isClosed())
                continue;
        }
        else if (qgraph::Line* line = dynamic_cast<qgraph::Line*>(item))
        {
            if (!line->isClosed())
                continue;
        }

        const QString className = item->data(0).toString();
        if (!className.isEmpty())
            roots.append(item);
    }

    std::sort(roots.begin(), roots.end(),
              [](QGraphicsItem* a, QGraphicsItem* b)
    {
        bool aOk = false;
        bool bOk = false;

        const int aOrder = a->data(_roleListOrder).toInt(&aOk);
        const int bOrder = b->data(_roleListOrder).toInt(&bOk);

        const int av = aOk ? aOrder : std::numeric_limits<int>::max();
        const int bv = bOk ? bOrder : std::numeric_limits<int>::max();

        return av < bv;
    });

    for (QGraphicsItem* item : roots)
        linkSceneItemToList(item, -1, false);

    renumberPolygonList();
}

void MainWindow::changeClassForSceneItem(QGraphicsItem* item)
{
    if (!item)
        return;

    Document::Ptr doc = currentDocument();
    if (!doc)
        return;

    QUndoStack* stack = activeUndoStack();
    if (!stack)
        return;

    const QString oldClass = item->data(0).toString();

    if (_projectClasses.isEmpty())
        return;

    SelectClass dialog(_projectClasses, this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    const QString newClass = dialog.selectedClass();
    if (newClass.isEmpty() || newClass == oldClass)
        return;

    undo::ChangeClass::Data data;
    data.classBefore = oldClass;
    data.classAfter = newClass;

    item->setData(0, newClass);
    applyClassColorToItem(item, newClass);

    if (polygonListRowByItem(item) < 0)
        linkSceneItemToList(item);
    else
        renumberPolygonList();

    if (item->scene())
        item->scene()->update();

    stack->push(new undo::ChangeClass(doc.get(),
                                      item,
                                      data,
                                      u8"Смена класса"));

    if (!doc->isModified)
    {
        doc->isModified = true;
        updateFileListDisplay(doc->filePath);
    }
}

void MainWindow::loadGeometry()
{
    std::vector<int> wsc23 = windowScreenCenter23();
    windowLoadGeometry(config::base(), "windows.main_window.geometry", this, &wsc23);

    QList<int> splitterSizes;
    if (config::base().getValue("windows.main_window.splitter_sizes", splitterSizes))
        ui->splitter->setSizes(splitterSizes);

    splitterSizes.clear();
    if (!config::base().getValue("windows.main_window.splitter2_sizes", splitterSizes))
    {
        splitterSizes = {100, 100};
        QRect windowGeometry = this->geometry();
        splitterSizes[0] = windowGeometry.width() * (2./3);
    }
    ui->splitter2->setSizes(splitterSizes);
}

void MainWindow::saveGeometry()
{
    QRect g = geometry();
    QVector<int> wg {g.x(), g.y(), g.width(), g.height()};
    config::base().setValue("windows.main_window.geometry", wg);

    QList<int> splitterSizes = ui->splitter->sizes();
    config::base().setValue("windows.main_window.splitter_sizes", splitterSizes);

    splitterSizes = ui->splitter2->sizes();
    config::base().setValue("windows.main_window.splitter2_sizes", splitterSizes);

//    config::base().setValue("windows.main_window.tab_index", ui->tabWidget->currentIndex());

//    QByteArray ba = ui->tableJournal->horizontalHeader()->saveState().toBase64();
//    config::base().setValue("windows.main_window.event_journal_header", QString::fromLatin1(ba));
    saveVisualStyle();
}

void MainWindow::writeShapesJsonToClipboard(const QJsonObject& json) const
{
    if (json.isEmpty())
        return;

    // JSON-данные - для внутренней вставки
    QJsonDocument doc(json);
    QByteArray raw = doc.toJson(QJsonDocument::Compact);

    QJsonArray shapesArray = json["shapes"].toArray();

    struct Circle { QString label; double x, y, radius; };
    struct Polyline { QString label; QVector<QPointF> points; };
    struct Rectangle{ QString label; double x1, y1, x2, y2; };
    struct Point { QString label; double x, y; };
    struct Line { QString label; QVector<QPointF> points; };

    QVector<Circle> circles;
    QVector<Polyline> polylines;
    QVector<Rectangle> rectangles;
    QVector<Point> points;
    QVector<Line> lines;

    for (const QJsonValue& v : shapesArray)
    {
        QJsonObject o = v.toObject();
        QString type  = o["type"].toString();
        QString label = o["class"].toString();
        if (label.isEmpty())
            label = "none";

        if (type == "circle")
        {
            Circle circle;
            circle.label  = label;
            circle.x = o["x"].toDouble();
            circle.y = o["y"].toDouble();
            circle.radius = o["radius"].toDouble();
            circles.append(circle);
        }
        else if (type == "rectangle")
        {
            Rectangle rect;
            rect.label = label;
            double x = o["x"].toDouble();
            double y = o["y"].toDouble();
            double w = o["width"].toDouble();
            double h = o["height"].toDouble();
            rect.x1 = x;
            rect.y1 = y;
            rect.x2 = x + w;
            rect.y2 = y + h;
            rectangles.append(rect);
        }
        else if (type == "polyline")
        {
            Polyline polyline;
            polyline.label = label;
            QJsonArray pts = o["points"].toArray();
            for (const QJsonValue& pv : pts)
            {
                QJsonObject po = pv.toObject();
                polyline.points.append(QPointF(po["x"].toDouble(),
                                         po["y"].toDouble()));
            }
            polylines.append(polyline);
        }
        else if (type == "line")
        {
            Line line;
            line.label = label;
            QJsonArray pts = o["points"].toArray();
            for (const QJsonValue& pv : pts)
            {
                QJsonObject po = pv.toObject();
                line.points.append(QPointF(po["x"].toDouble(),
                                         po["y"].toDouble()));
            }
            lines.append(line);
        }
        else if (type == "point")
        {
            Point point;
            point.label = label;
            point.x = o["x"].toDouble();
            point.y = o["y"].toDouble();
            points.append(point);
        }
    }

    QString yaml;
    {
        QTextStream out(&yaml);
        out << "shapes:\n";

        if (!circles.isEmpty())
        {
            out << "  circles:\n";
            for (const Circle& c : circles)
            {
                int cx = static_cast<int>(std::round(c.x));
                int cy = static_cast<int>(std::round(c.y));
                int r  = static_cast<int>(std::round(c.radius));

                out << "    - label: " << c.label << "\n";
                out << "      center: {x: " << cx << ", y: " << cy << "}\n";
                out << "      radius: " << r << "\n";
            }
        }

        if (!polylines.isEmpty())
        {
            out << "  polylines:\n";
            for (const Polyline& polyline : polylines)
            {
                out << "    - label: " << polyline.label << "\n";
                out << "      points: [";
                for (int i = 0; i < polyline.points.size(); ++i)
                {
                    const QPointF& point = polyline.points[i];
                    int px = static_cast<int>(std::round(point.x()));
                    int py = static_cast<int>(std::round(point.y()));
                    if (i > 0)
                        out << ", ";
                    out << "{x: " << px << ", y: " << py << "}";
                }
                out << "]\n";
            }
        }

        if (!rectangles.isEmpty())
        {
            out << "  rectangles:\n";
            for (const Rectangle& r : rectangles)
            {
                int x1 = static_cast<int>(std::round(r.x1));
                int y1 = static_cast<int>(std::round(r.y1));
                int x2 = static_cast<int>(std::round(r.x2));
                int y2 = static_cast<int>(std::round(r.y2));

                out << "    - label: " << r.label << "\n";
                out << "      point1: {x: " << x1 << ", y: " << y1 << "}\n";
                out << "      point2: {x: " << x2 << ", y: " << y2 << "}\n";
            }
        }

        if (!points.isEmpty())
        {
            out << "  points:\n";
            for (const Point& p : points)
            {
                int px = static_cast<int>(std::round(p.x));
                int py = static_cast<int>(std::round(p.y));

                out << "    - label: " << p.label << "\n";
                out << "      point: {x: " << px << ", y: " << py << "}\n";
            }
        }

        if (!lines.isEmpty())
        {
            out << "  lines:\n";
            for (const Line& ln : lines)
            {
                out << "    - label: " << ln.label << "\n";
                out << "      points: [";
                for (int i = 0; i < ln.points.size(); ++i)
                {
                    const QPointF& p = ln.points[i];
                    int px = static_cast<int>(std::round(p.x()));
                    int py = static_cast<int>(std::round(p.y()));
                    if (i > 0)
                        out << ", ";
                    out << "{x: " << px << ", y: " << py << "}";
                }
                out << "]\n";
            }
        }
    }

    // Пишем и JSON и YAML в буфер обмена
    QMimeData* mime = new QMimeData;
    mime->setData(kShapesMimeType, raw);
    mime->setText(yaml);

    QClipboard* cb = QGuiApplication::clipboard();
    cb->setMimeData(mime);
}

QJsonObject MainWindow::readShapesJsonFromClipboard() const
{
    QClipboard* cb = QGuiApplication::clipboard();
    const QMimeData* mime = cb->mimeData();
    if (!mime)
        return {};

    QByteArray raw;

    // Сначала пробуем наш MIME-тип
    if (mime->hasFormat(kShapesMimeType))
    {
        raw = mime->data(kShapesMimeType);
    }
    else if (mime->hasText())
    {
        // fallback: вдруг пользователь сам сохранил JSON в виде текста
        raw = mime->text().toUtf8();
    }
    else
    {
        return {};
    }

    QJsonParseError err{};
    QJsonDocument doc = QJsonDocument::fromJson(raw, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return {};

    return doc.object();
}

QJsonObject MainWindow::serializeSceneToJson(QGraphicsScene* scene)
{
    QJsonObject root;
    QJsonArray shapesArray;

    Document::Ptr doc = currentDocument();

    if (!scene)
    {
        root["shapes"] = shapesArray;
        return root;
    }

    for (QGraphicsItem* item : scene->items())
    {
        // Пропускаем само изображение и временные элементы
        if (item == doc->videoRect
            || item == _tempRectItem
            || item == _tempCircleItem
            || item == _tempPolyline)
        {
            continue;
        }

        QJsonObject shapeObj;
        QString className = item->data(0).toString();
        shapeObj["class"] = className;
        // Пропускаем элементы без класса
        if (className.isEmpty() || className == "--")
        {
            continue;
        }

        if (qgraph::Rectangle* rect = dynamic_cast<qgraph::Rectangle*>(item))
        {
            shapeObj["type"] = "rectangle";
            // QRectF r = rect->rect();
            QRectF r = rect->sceneBoundingRect();
            shapeObj["x"] = r.x();
            shapeObj["y"] = r.y();
            shapeObj["width"] = r.width();
            shapeObj["height"] = r.height();
        }
        else if (qgraph::Circle* circle = dynamic_cast<qgraph::Circle*>(item))
        {
            shapeObj["type"] = "circle";
            //QPointF center = circle->realCenter();
            QPointF center = circle->center();
            shapeObj["x"] = center.x();
            shapeObj["y"] = center.y();
            shapeObj["radius"] = circle->realRadius();
        }
        else if (qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(item))
        {
            shapeObj["type"] = "polyline";
            shapeObj["closed"] = polyline->isClosed();
            QJsonArray pointsArray;

            // Получаем точки полилинии через метод points()
            for (const QPointF& point : polyline->points())
            {
                QJsonObject pointObj;
                pointObj["x"] = point.x();
                pointObj["y"] = point.y();
                pointsArray.append(pointObj);
            }
            shapeObj["points"] = pointsArray;
        }
        else if (qgraph::Line* line = dynamic_cast<qgraph::Line*>(item))
        {
            shapeObj["type"] = "line";
            QJsonArray pointsArray;

            for (const QPointF& point : line->points())
            {
                QJsonObject pointObj;
                pointObj["x"] = point.x();
                pointObj["y"] = point.y();
                pointsArray.append(pointObj);
            }

            shapeObj["points"] = pointsArray;
        }
        else if (qgraph::Point* point = dynamic_cast<qgraph::Point*>(item))
        {
            shapeObj["type"] = "point";
            QPointF center = point->center();
            shapeObj["x"] = center.x();
            shapeObj["y"] = center.y();
        }
        else
        {
            continue;
        }

        shapesArray.append(shapeObj);
    }

    root["shapes"] = shapesArray;
    return root;
}

void MainWindow::deserializeJsonToScene(QGraphicsScene* scene,
                                        const QJsonObject& json,
                                        const QPointF& offset)
{
    if (!scene)
    {
        return;
    }

    QJsonArray shapesArray = json["shapes"].toArray();
    for (const QJsonValue& shapeValue : shapesArray)
    {
        QJsonObject shapeObj = shapeValue.toObject();
        QString className = shapeObj["class"].toString();
        QString type = shapeObj["type"].toString();

        if (type == "rectangle")
        {
            qreal x = shapeObj["x"].toDouble();
            qreal y = shapeObj["y"].toDouble();
            qreal width = shapeObj["width"].toDouble();
            qreal height = shapeObj["height"].toDouble();

            QPointF topLeft(x, y);
            topLeft += offset;

            qgraph::Rectangle* rect = new qgraph::Rectangle(scene);

            ensureUid(rect);
            rect->setRealSceneRect(QRectF(topLeft, QSizeF(width, height)));
            rect->setData(0, className);
            apply_LineWidth_ToItem(rect);
            apply_PointSize_ToItem(rect);
            apply_NumberSize_ToItem(rect);
        }
        else if (type == "circle")
        {
            qreal x = shapeObj["x"].toDouble();
            qreal y = shapeObj["y"].toDouble();
            qreal radius = shapeObj["radius"].toDouble();

            QPointF center(x, y);
            center += offset;

            qgraph::Circle* circle = new qgraph::Circle(scene, center);

            ensureUid(circle);
            circle->setRealRadius(radius);
            circle->setData(0, className);
            apply_LineWidth_ToItem(circle);
            apply_PointSize_ToItem(circle);
            apply_NumberSize_ToItem(circle);
        }
        else if (type == "polyline")
        {
            QJsonArray pointsArray = shapeObj["points"].toArray();
            if (pointsArray.isEmpty())
                continue;

            const bool closed = shapeObj["closed"].toBool(false);

            QPointF firstPoint(pointsArray[0].toObject()["x"].toDouble(),
                               pointsArray[0].toObject()["y"].toDouble());

            firstPoint += offset;

            qgraph::Polyline* polyline = new qgraph::Polyline(scene, firstPoint);
            ensureUid(polyline);

            polyline->beginBulkLoad();
            for (int i = 1; i < pointsArray.size(); ++i)
            {
                QPointF point(pointsArray[i].toObject()["x"].toDouble(),
                              pointsArray[i].toObject()["y"].toDouble());
                point += offset;
                polyline->addPoint(point, scene);
            }
            polyline->endBulkLoad();

            if (closed)
                polyline->closePolyline();

            polyline->setData(0, className);
            apply_LineWidth_ToItem(polyline);
            apply_PointSize_ToItem(polyline);
            apply_NumberSize_ToItem(polyline);
        }
        else if (type == "line")
        {
            QJsonArray pointsArray = shapeObj["points"].toArray();
            if (pointsArray.size() < 2)
                continue;

            QPointF firstPoint(pointsArray[0].toObject()["x"].toDouble(),
                               pointsArray[0].toObject()["y"].toDouble());
            firstPoint += offset;

            qgraph::Line* line = new qgraph::Line(scene, firstPoint);
            ensureUid(line);

            line->beginBulkLoad();
            for (int i = 1; i < pointsArray.size(); ++i)
            {
                QJsonObject ptObj = pointsArray[i].toObject();
                QPointF p(ptObj["x"].toDouble(), ptObj["y"].toDouble());
                p += offset;
                line->addPoint(p, scene);
            }
            line->endBulkLoad();

            line->setData(0, className);

            apply_LineWidth_ToItem(line);
            apply_PointSize_ToItem(line);
            apply_NumberSize_ToItem(line);
        }
        else if (type == "point")
        {
            qreal x = shapeObj["x"].toDouble();
            qreal y = shapeObj["y"].toDouble();

            QPointF center(x, y);
            center += offset;

            qgraph::Point* p = new qgraph::Point(scene);

            ensureUid(p);
            p->setCenter(center);
            p->setData(0, className);

            apply_PointSize_ToItem(p);
            apply_NumberSize_ToItem(p);
            apply_PointStyle_ToItem(p);
        }
        else
        {
            continue;
        }
    }
}

QJsonObject MainWindow::serializeSelectedItemsToJson(QGraphicsScene* scene)
{
    Document::Ptr doc = currentDocument();
    qgraph::VideoRect* videoRect = doc->videoRect;

    QJsonObject root;
    QJsonArray shapesArray;

    if (!scene)
    {
        root["shapes"] = shapesArray;
        return root;
    }

    // Берем именно выделенные элементы
    const QList<QGraphicsItem*> items = scene->selectedItems();

    for (QGraphicsItem* item : items)
    {
        // Пропускаем само изображение и временные элементы
        if (item == videoRect || item == _tempRectItem ||
            item == _tempCircleItem || item == _tempPolyline)
        {
            continue;
        }

        QJsonObject shapeObj;
        QString className = item->data(0).toString();

        // Пропускаем элементы без класса
        if (className.isEmpty() || className == "--")
        {
            continue;
        }

        shapeObj["class"] = className;

        if (qgraph::Rectangle* rect = dynamic_cast<qgraph::Rectangle*>(item))
        {
            shapeObj["type"] = "rectangle";
            //QRectF r = rect->rect();
            QRectF r = rect->sceneBoundingRect();
            shapeObj["x"] = r.x();
            shapeObj["y"] = r.y();
            shapeObj["width"] = r.width();
            shapeObj["height"] = r.height();
        }
        else if (qgraph::Circle* circle = dynamic_cast<qgraph::Circle*>(item))
        {
            shapeObj["type"] = "circle";
            //QPointF center = circle->realCenter();
            QPointF center = circle->center();
            shapeObj["x"] = center.x();
            shapeObj["y"] = center.y();
            shapeObj["radius"] = circle->realRadius();
        }
        else if (qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(item))
        {
            shapeObj["type"] = "polyline";
            shapeObj["closed"] = polyline->isClosed();
            QJsonArray pointsArray;

            for (const QPointF& point : polyline->points())
            {
                QJsonObject pt;
                pt["x"] = point.x();
                pt["y"] = point.y();
                pointsArray.append(pt);
            }

            shapeObj["points"] = pointsArray;
        }
        else if (qgraph::Line* line = dynamic_cast<qgraph::Line*>(item))
        {
            shapeObj["type"] = "line";
            QJsonArray pointsArray;

            for (const QPointF& point : line->points())
            {
                QJsonObject pt;
                pt["x"] = point.x();
                pt["y"] = point.y();
                pointsArray.append(pt);
            }

            shapeObj["points"] = pointsArray;
        }
        else if (qgraph::Point* point = dynamic_cast<qgraph::Point*>(item))
        {
            shapeObj["type"] = "point";
            QPointF center = point->center();
            shapeObj["x"] = center.x();
            shapeObj["y"] = center.y();
        }
        else
        {
            continue;
        }

        shapesArray.append(shapeObj);
    }

    root["shapes"] = shapesArray;
    return root;
}

void MainWindow::copySelectedShapes()
{
    Document::Ptr doc = currentDocument();
    if (!doc || !doc->scene)
        return;

    // Собираем JSON только по выделенным фигурам на текущей сцене
    QJsonObject json = serializeSelectedItemsToJson(doc->scene);

    // Запоминаем снимок, с которого были скопированы примитивы
    json["sourceImagePath"] = QFileInfo(doc->filePath).absoluteFilePath();

    // Локальный буфер
    _shapesClipboard = json;

    // Отправляем JSON в системный буфер обмена
    writeShapesJsonToClipboard(json);
}

void MainWindow::pasteCopiedShapesToCurrentScene()
{
    Document::Ptr doc = currentDocument();
    QGraphicsScene* scene = doc->scene;
    qgraph::VideoRect* videoRect = doc->videoRect;

    if (!doc) return;

    QJsonObject json = readShapesJsonFromClipboard();
    if (json.isEmpty())
        json = _shapesClipboard;

    if (json.isEmpty())
        return;

    // Запоминаем, что было на сцене ДО вставки
    QList<QGraphicsItem*> beforeItems = scene->items();

    QString (*normalizedImagePath)(const QString&) = [](const QString& path) -> QString
    {
        if (path.isEmpty())
            return {};

        return QDir::cleanPath(QFileInfo(path).absoluteFilePath());
    };

    const QString sourcePath = normalizedImagePath(json["sourceImagePath"].toString());
    const QString currentPath = normalizedImagePath(doc->filePath);

    #ifdef Q_OS_WIN
    const bool sameImage = !sourcePath.isEmpty() &&
                           QString::compare(sourcePath, currentPath, Qt::CaseInsensitive) == 0;
    #else
    const bool sameImage = !sourcePath.isEmpty() && sourcePath == currentPath;
    #endif

    QPointF pasteOffset;

    if (sameImage)
    {
        QSize imageSize = doc->pixmap.size();

        if (!imageSize.isValid() && videoRect)
            imageSize = videoRect->pixmap().size();

        const qreal imageSide = qMin(imageSize.width(), imageSize.height());

        // Сдвиг равен 2% от меньшей стороны изображения
        const qreal offsetValue = qBound<qreal>(10.0, imageSide * 0.02, 250.0);

        pasteOffset = QPointF(offsetValue, offsetValue);
    }

    deserializeJsonToScene(scene, json, pasteOffset);

    // Собираем список новых фигур (тех, которых не было в beforeItems)
    QList<QGraphicsItem*> afterItems = scene->items();
    QList<QGraphicsItem*> newItems;

    for (QGraphicsItem* item : afterItems)
    {
        if (!item)
            continue;

        if (item == videoRect ||
            item == _tempRectItem ||
            item == _tempCircleItem ||
            item == _tempPolyline)
        {
            continue;
        }

        if (item->parentItem() != nullptr)
            continue;

        if (beforeItems.contains(item))
            continue;

        if (dynamic_cast<qgraph::Rectangle*>(item) ||
            dynamic_cast<qgraph::Circle*>(item) ||
            dynamic_cast<qgraph::Polyline*>(item) ||
            dynamic_cast<qgraph::Line*>(item) ||
            dynamic_cast<qgraph::Point*>(item))
        {
            newItems.append(item);
        }
    }

    if (newItems.isEmpty())
        return;

    // После вставки активными должны стать именно новые фигуры
    const QList<QGraphicsItem*> selectedBeforePaste = scene->selectedItems();
    for (QGraphicsItem* item : selectedBeforePaste)
    {
        if (item)
            item->setSelected(false);
    }

    for (QGraphicsItem* item : newItems)
    {
        if (item)
            item->setSelected(true);
    }

    if (!newItems.isEmpty() && newItems.last())
        newItems.last()->setFocus();

    raiseAllHandlesToTop();

    // Новые фигуры должны быть выше уже существующих на сцене
    qreal maxZ = 0.0;
    bool hasExisting = false;

    for (QGraphicsItem* item : beforeItems)
    {
        if (!item)
            continue;

        if (item == videoRect ||
            item == _tempRectItem ||
            item == _tempCircleItem ||
            item == _tempPolyline)
        {
            continue;
        }

        if (item->parentItem() != nullptr)
            continue;

        maxZ = hasExisting ? qMax(maxZ, item->zValue()) : item->zValue();
        hasExisting = true;
    }

    if (!hasExisting)
        maxZ = 0.0;

    for (int i = 0; i < newItems.size(); ++i)
    {
        if (newItems[i])
            newItems[i]->setZValue(maxZ + 1.0 + i);
    }

    // Добавляем новые фигуры в конец списка справа
    QList<QListWidgetItem*> pastedListItems;
    pastedListItems.reserve(newItems.size());

    QModelIndexList pastedIndexes;

    for (QGraphicsItem* item : newItems)
    {
        if (!item)
            continue;

        const QString className = item->data(0).toString();
        if (className.isEmpty())
            continue;

        linkSceneItemToList(item, -1, false);

        const int row = polygonListRowByItem(item);
        //if (doc && doc->polygonList.model && row >= 0)
        if (row >= 0)
            pastedIndexes.append(doc->polygonList.model.index(row, 0));
    }

    renumberPolygonListTextOnly();

    {
        QSignalBlocker blocker(ui->polygonList);
        ui->polygonList->clearSelection();

        QItemSelectionModel* selectionModel = ui->polygonList->selectionModel();

        if (selectionModel)
        {
            for (const QModelIndex& index : pastedIndexes)
            {
                selectionModel->select(index,
                                       QItemSelectionModel::Select | QItemSelectionModel::Rows);
            }

            if (!pastedIndexes.isEmpty())
                ui->polygonList->setCurrentIndex(pastedIndexes.last());
        }
    }

    ui->polygonList->setFocus();

    QString descr = (newItems.count() == 1) ? u8"Вставка примитива"
                                            : u8"Вставка примитивов";
    if (QUndoStack* stack = activeUndoStack())
    {
        stack->push(new undo::Paste(doc.get(),
                                    newItems,
                                    descr));
    }
    doc->isModified = true;
    updateFileListDisplay(doc->filePath);
}

qgraph::VideoRect* MainWindow::findVideoRect(QGraphicsScene* scene)
{
    if (!scene)
    {
        return nullptr;
    }

    foreach(QGraphicsItem* item, scene->items())
    {
        if (qgraph::VideoRect* videoRect = dynamic_cast<qgraph::VideoRect*>(item))
        {
            return videoRect;
        }
    }
    return nullptr;
}

void MainWindow::toggleRightSplitter()
{
    QSplitter* rightSplitter = ui->splitter;

    if (_isRightSplitterCollapsed)
    {
        // Восстанавливаем предыдущие размеры
        rightSplitter->setSizes(_savedSplitterSizes);
        rightSplitter->setVisible(true);
    }
    else
    {
        // Сохраняем текущие размеры перед схлопыванием
        _savedSplitterSizes = rightSplitter->sizes();

        // Схлопываем сплиттер (устанавливаем все размеры в 0)
        QList<int> newSizes;
        for (int i = 0; i < rightSplitter->count(); ++i)
            newSizes << 0;
        rightSplitter->setSizes(newSizes);
    }

    _isRightSplitterCollapsed = !_isRightSplitterCollapsed;
}

void MainWindow::updateWindowTitle()
{
    QString title;

    if (!_currentFolderPath.isEmpty())
    {
        // Форматируем путь для отображения
        QString displayPath = QDir::toNativeSeparators(_currentFolderPath);

        // Если путь длинный, можно сократить его
        if (displayPath.length() > 50)
        {
            displayPath = "..." + displayPath.right(47);
        }

        title = QString("%1 ― QLabelMe").arg(displayPath);
    }
    else
    {
        title = "QLabelMe";
    }

    setWindowTitle(title);
}

QString MainWindow::annotationPathFor(const QString& imagePath)
{
    QFileInfo fileInfo(imagePath);
    QDir dir(fileInfo.absolutePath());
    return dir.filePath(fileInfo.completeBaseName() + ".yaml");
}

double MainWindow::round2(double value)
{
    return std::round(value * 100.0) / 100.0;
}

void MainWindow::saveAnnotationToFile(Document::Ptr doc)
{
    if (!doc || !doc->scene || !doc->videoRect)
        return;

    void (*savePointNode)(YamlConfig*, YAML::Node&, const char*, const QPointF&) =
        [](YamlConfig* conf, YAML::Node& parent, const char* key, const QPointF& point)
    {
        YamlConfig::Func savePoint = [point](YamlConfig* conf, YAML::Node& ypoint, bool)
        {
            conf->setValue(ypoint, "x", static_cast<double>(point.x()));
            conf->setValue(ypoint, "y", static_cast<double>(point.y()));
            return true;
        };

        conf->setValue(parent, key, savePoint);
    };

    void (*savePointsArray)(YamlConfig*, YAML::Node&, const QVector<QPointF>&) =
        [](YamlConfig* conf, YAML::Node& parent, const QVector<QPointF>& points)
    {
        YamlConfig::Func savePoints = [points](YamlConfig* conf, YAML::Node& ypoints, bool)
        {
            for (const QPointF& point : points)
            {
                YAML::Node ypoint;

                conf->setValue(ypoint, "x", static_cast<double>(point.x()));
                conf->setValue(ypoint, "y", static_cast<double>(point.y()));

                ypoints.push_back(ypoint);
            }

            return true;
        };

        conf->setValue(parent, "points", savePoints);
        conf->setNodeStyle(parent, "points", YAML::EmitterStyle::Flow);
    };

    YamlConfig::Func saveShapes = [&](YamlConfig* conf, YAML::Node& yshapes, bool)
    {
        const QList<QGraphicsItem*> items = orderedShapeItemsForSave(doc);

        for (QGraphicsItem* item : items)
        {
            if (!item)
                continue;

            YAML::Node yshape;

            QString className = item->data(0).toString();
            if (className.isEmpty())
                className = "none";

            conf->setValue(yshape, "number", ensureShapeNumber(item));
            conf->setValue(yshape, "label", className);

            if (qgraph::Line* line = dynamic_cast<qgraph::Line*>(item))
            {
                conf->setValue(yshape, "type", QString("line"));
                savePointsArray(conf, yshape, line->points());
                conf->setValue(yshape, "numbering_from_last", line->isNumberingFromLast());
            }
            else if (qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(item))
            {
                conf->setValue(yshape, "type", QString("polyline"));
                savePointsArray(conf, yshape, polyline->points());
            }
            else if (qgraph::Rectangle* rect = dynamic_cast<qgraph::Rectangle*>(item))
            {
                conf->setValue(yshape, "type", QString("rectangle"));

                const QRectF sceneRect = rect->realSceneRect();
                savePointNode(conf, yshape, "point1", sceneRect.topLeft());
                savePointNode(conf, yshape, "point2", sceneRect.bottomRight());
                conf->setValue(yshape, "numbering_offset", rect->numberingOffset());
            }
            else if (qgraph::Circle* circle = dynamic_cast<qgraph::Circle*>(item))
            {
                conf->setValue(yshape, "type", QString("circle"));

                savePointNode(conf, yshape, "center", circle->realCenter());
                conf->setValue(yshape, "radius", static_cast<double>(circle->realRadius()));
            }
            else if (qgraph::Point* point = dynamic_cast<qgraph::Point*>(item))
            {
                conf->setValue(yshape, "type", QString("point"));
                savePointNode(conf, yshape, "point", point->center());
            }
            else
            {
                continue;
            }

            yshapes.push_back(yshape);
        }

        return true;
    };

    YamlConfig yconfig;
    yconfig.setRounding(2);
    yconfig.setValue("shapes", saveShapes);

    const QString yamlPath = annotationPathFor(doc->filePath);
    const QByteArray encoded = QFile::encodeName(QDir::toNativeSeparators(yamlPath));

    const bool saved = yconfig.saveFile(std::string(encoded.constData(), encoded.size()));
    if (!saved)
    {
        log_error_m << "Ошибка при сохранении разметки:" << yamlPath;

        messageBox(
            this,
            QMessageBox::Warning,
            u8"Не удалось сохранить файл разметки"
        );

        return;
    }

    if (doc->_undoStack)
        doc->_undoStack->setClean();

    // После успешного сохранения
    doc->isModified = false;
    updateFileListDisplay(doc->filePath);
}

void MainWindow::updateFileListDisplay(const QString& filePath)
{
    QIcon modifiedIcon(":/images/resources/not_ok.svg");     // Красная иконка - есть изменения
    QIcon savedIcon(":/images/resources/ok.svg");            // Зеленая иконка - сохранено
    QIcon noAnnotationIcon(":/images/resources/not_ok.svg"); // Красная иконка - нет аннотаций

    for (int i = 0; i < ui->fileList->count(); ++i)
    {
        QListWidgetItem* item = ui->fileList->item(i);
        QVariant data = item->data(Qt::UserRole);
        if (data.canConvert<Document::Ptr>())
        {
            Document::Ptr itemDoc = data.value<Document::Ptr>();
            if (itemDoc->filePath == filePath)
            {
                QString fileName = QFileInfo(filePath).fileName();

                if (itemDoc->isModified)
                {
                    // Документ изменен, но не сохранен
                    item->setText("* " + fileName);
                    item->setIcon(modifiedIcon);
                }
                else if (hasAnnotationFile(filePath))
                {
                    // Документ сохранен, проверяем не пустой ли YAML
                    QString yamlPath = getAnnotationFilePath(filePath);
                    if (!isYamlFileEmpty(yamlPath))
                    {
                        item->setText(fileName);
                        item->setIcon(savedIcon);
                    }
                    else
                    {
                        item->setText(fileName);
                        item->setIcon(noAnnotationIcon);
                    }
                }
                else
                {
                    // Нет файла аннотаций
                    item->setText(fileName);
                    item->setIcon(noAnnotationIcon);
                }
                break;
            }
        }
    }
}

void MainWindow::loadAnnotationFromFile(Document::Ptr doc, bool rebuildUi)
{
    if (!doc || !doc->scene)
    {
        return;
    }

    // Временно блокируем обработку изменений
    _loadingNow = true;
    QSignalBlocker blocker(doc->scene); // временно глушим QGraphicsScene::changed
    // Сдвиг изображения, если восстанавливаем разметку
    const QPointF imageOffset = (doc->videoRect ? doc->videoRect->pos() : QPointF(0.0, 0.0));

    const QString yamlPath = annotationPathFor(doc->filePath);
    YamlConfig yconfig;

    const QByteArray encoded = QFile::encodeName(QDir::toNativeSeparators(yamlPath));
    if (!yconfig.readFile(std::string(encoded.constData(), encoded.size())))
    {
        log_error_m << "Ошибка при загрузке разметки:" << yamlPath;
        _loadingNow = false;
        return;
    }

    // Очищаем сцену только если нет несохраненных изменений
    if (!doc->isModified)
    {
        const QList<QGraphicsItem*> items = doc->scene->items();
        for (QGraphicsItem* item : items)
        {
            if (!item)
                continue;

            if (item == doc->videoRect || item == doc->pixmapItem)
                continue;

            if (item == _ghostHandle)
                continue;

            // Не удаляем дочерние элементы
            if (item->parentItem() != nullptr)
                continue;

            doc->scene->removeItem(item);
            delete item; // Удалит и всех детей автоматически
        }
    }

    int fallbackOrder = 0;

    std::function<void(QGraphicsItem*)> finalizeOldLoadedShape = [&](QGraphicsItem* item)
    {
        if (!item)
            return;

        item->setData(_roleShapeNumber, fallbackOrder);
        item->setData(_roleListOrder, fallbackOrder);

        const qreal baseZ = doc->videoRect ? doc->videoRect->zValue() : 0.0;
        item->setZValue(baseZ + 1.0 + fallbackOrder);

        ++fallbackOrder;
    };

    YamlConfig::Func loadCircles = [&](YamlConfig* conf, YAML::Node& ycircles, bool)
    {
        for (const YAML::Node& ycircle : ycircles)
        {
            QString label;
            conf->getValue(ycircle, "label", label);

            double centerX = 0.0;
            double centerY = 0.0;
            double radius = 0.0;

            if (ycircle["center"])
            {
                conf->getValue(ycircle["center"], "x", centerX);
                conf->getValue(ycircle["center"], "y", centerY);
            }
            conf->getValue(ycircle, "radius", radius);

            qgraph::Circle* circle = new qgraph::Circle(doc->scene, QPointF(centerX, centerY) + imageOffset);
            ensureUid(circle);
            circle->setRealRadius(radius);
            circle->setData(0, label);
            apply_LineWidth_ToItem(circle);
            apply_PointSize_ToItem(circle);
            applyClassColorToItem(circle, label);
            finalizeOldLoadedShape(circle);
        }
        return true;
    };
    YamlConfig::Func loadPolylines = [&](YamlConfig* conf, YAML::Node& ypolylines, bool)
    {
        for (const YAML::Node& ypolyline : ypolylines)
        {
            QString label;
            conf->getValue(ypolyline, "label", label);

            QVector<QPointF> points;
            YamlConfig::Func loadPoints = [&points, &imageOffset](YamlConfig* conf, YAML::Node& ypoints, bool)
            {
                for (const YAML::Node& ypoint : ypoints)
                {
                    double x = 0.0;
                    double y = 0.0;
                    conf->getValue(ypoint, "x", x);
                    conf->getValue(ypoint, "y", y);
                    points.append(QPointF(x, y) + imageOffset);
                }
                return true;
            };
            conf->getValue(ypolyline, "points", loadPoints);

            if (points.isEmpty())
                continue;
            // Создаем полилинию на сцене
            qgraph::Polyline* polyline = new qgraph::Polyline(doc->scene, points.first());
            ensureUid(polyline);

            polyline->beginBulkLoad();
            for (int i = 1; i < points.size(); ++i)
            {
                polyline->addPoint(points[i], doc->scene);
            }
            polyline->endBulkLoad();

            polyline->closePolyline();
            polyline->updatePath();
            polyline->setData(0, label);
            apply_LineWidth_ToItem(polyline);
            apply_PointSize_ToItem(polyline);
            apply_NumberSize_ToItem(polyline);
            applyClassColorToItem(polyline, label);
            finalizeOldLoadedShape(polyline);
        }
        return true;
    };
    YamlConfig::Func loadRectangles = [&](YamlConfig* conf, YAML::Node& yrectangles, bool)
    {
        for (const YAML::Node& yrectangle : yrectangles)
        {
            QString label;
            conf->getValue(yrectangle, "label", label);

            double x1 = 0.0, y1 = 0.0;
            double x2 = 0.0, y2 = 0.0;

            if (yrectangle["point1"])
            {
                conf->getValue(yrectangle["point1"], "x", x1);
                conf->getValue(yrectangle["point1"], "y", y1);
            }
            if (yrectangle["point2"])
            {
                conf->getValue(yrectangle["point2"], "x", x2);
                conf->getValue(yrectangle["point2"], "y", y2);
            }
            // Создаем прямоугольник на сцене
            qgraph::Rectangle* rect = new qgraph::Rectangle(doc->scene);
            ensureUid(rect);
            rect->setRealSceneRect(QRectF(QPointF(x1, y1) + imageOffset,
                                          QPointF(x2, y2) + imageOffset));
            rect->setData(0, label);
            apply_LineWidth_ToItem(rect);
            apply_PointSize_ToItem(rect);
            apply_NumberSize_ToItem(rect);
            applyClassColorToItem(rect, label);
            finalizeOldLoadedShape(rect);
        }

        return true;
    };
    YamlConfig::Func loadPoints = [&](YamlConfig* conf, YAML::Node& ypoints, bool)
    {
        for (const YAML::Node& ypoint : ypoints)
        {
            QString label;
            conf->getValue(ypoint, "label", label);

            double x = 0.0;
            double y = 0.0;

            if (ypoint["point"])
            {
                conf->getValue(ypoint["point"], "x", x);
                conf->getValue(ypoint["point"], "y", y);
            }

            qgraph::Point* p = new qgraph::Point(doc->scene);
            ensureUid(p);
            p->setCenter(QPointF(x, y) + imageOffset);
            p->setData(0, label);

            apply_PointSize_ToItem(p);
            apply_NumberSize_ToItem(p);
            apply_PointStyle_ToItem(p);
            applyClassColorToItem(p, label);
            finalizeOldLoadedShape(p);
        }
        return true;
    };
    YamlConfig::Func loadLines = [&](YamlConfig* conf, YAML::Node& ylines, bool)
    {
        for (const YAML::Node& yline : ylines)
        {
            QString label;
            conf->getValue(yline, "label", label);

            QVector<QPointF> points;
            YamlConfig::Func loadPoints = [&points, &imageOffset](YamlConfig* conf, YAML::Node& ypoints, bool)
            {
                for (const YAML::Node& ypoint : ypoints)
                {
                    double x = 0.0;
                    double y = 0.0;
                    conf->getValue(ypoint, "x", x);
                    conf->getValue(ypoint, "y", y);
                    points.append(QPointF(x, y) + imageOffset);
                }
                return true;
            };
            conf->getValue(yline, "points", loadPoints);

            if (points.isEmpty())
                continue;
            // Создаем линию на сцене
            qgraph::Line* line = new qgraph::Line(doc->scene, points.first());
            ensureUid(line);

            line->beginBulkLoad();
            for (int i = 1; i < points.size(); ++i)
            {
                line->addPoint(points[i], doc->scene);
            }
            line->endBulkLoad();

            line->closeLine();
            line->updatePath();
            line->setData(0, label);
            apply_LineWidth_ToItem(line);
            apply_PointSize_ToItem(line);
            apply_NumberSize_ToItem(line);
            applyClassColorToItem(line, label);
            finalizeOldLoadedShape(line);
        }
        return true;
    };

    YamlConfig::Func loadShapes = [&](YamlConfig* conf, YAML::Node& yshapes, bool)
    {
        int order = 0;
        const qreal baseZ = doc->videoRect ? doc->videoRect->zValue() : 0.0;

        std::function<bool(const YAML::Node&, const char*, QPointF&)> readPointNode =
            [&](const YAML::Node& parent, const char* key, QPointF& point) -> bool
        {
            if (!parent[key])
                return false;

            double x = 0.0;
            double y = 0.0;

            conf->getValue(parent[key], "x", x);
            conf->getValue(parent[key], "y", y);

            point = QPointF(x, y) + imageOffset;
            return true;
        };

        std::function<void(const YAML::Node&, QVector<QPointF>&)> readPointsArray =
            [&](const YAML::Node& parent, QVector<QPointF>& points)
        {
            if (!parent["points"])
                return;

            for (const YAML::Node& ypoint : parent["points"])
            {
                double x = 0.0;
                double y = 0.0;

                conf->getValue(ypoint, "x", x);
                conf->getValue(ypoint, "y", y);

                points.append(QPointF(x, y) + imageOffset);
            }
        };

        std::function<void(QGraphicsItem*, const YAML::Node&, const QString&)> finalizeShape =
            [&](QGraphicsItem* item, const YAML::Node& yshape, const QString& label)
        {
            if (!item)
                return;

            int number = order;
            if (yshape["number"])
                conf->getValue(yshape, "number", number);

            item->setData(0, label);
            item->setData(_roleShapeNumber, number);
            item->setData(_roleListOrder, order);
            // Визуальный порядок восстанавливаем по порядку элементов в shapes
            item->setZValue(baseZ + 1.0 + order);

            ++order;
        };

        for (const YAML::Node& yshape : yshapes)
        {
            QString type;
            QString label;

            conf->getValue(yshape, "type", type);
            conf->getValue(yshape, "label", label);

            if (label.isEmpty())
                label = "none";

            QGraphicsItem* created = nullptr;

            if (type == "circle")
            {
                QPointF center;
                double radius = 0.0;

                readPointNode(yshape, "center", center);
                conf->getValue(yshape, "radius", radius);

                qgraph::Circle* circle = new qgraph::Circle(doc->scene, center);
                ensureUid(circle);
                circle->setRealRadius(radius);

                apply_LineWidth_ToItem(circle);
                apply_PointSize_ToItem(circle);
                applyClassColorToItem(circle, label);

                created = circle;
            }
            else if (type == "polyline")
            {
                QVector<QPointF> points;
                readPointsArray(yshape, points);

                if (points.isEmpty())
                    continue;

                qgraph::Polyline* polyline = new qgraph::Polyline(doc->scene, points.first());
                ensureUid(polyline);

                polyline->beginBulkLoad();
                for (int i = 1; i < points.size(); ++i)
                    polyline->addPoint(points[i], doc->scene);
                polyline->endBulkLoad();

                polyline->closePolyline();
                polyline->updatePath();

                apply_LineWidth_ToItem(polyline);
                apply_PointSize_ToItem(polyline);
                apply_NumberSize_ToItem(polyline);
                applyClassColorToItem(polyline, label);

                created = polyline;
            }
            else if (type == "rectangle")
            {
                QPointF p1;
                QPointF p2;

                readPointNode(yshape, "point1", p1);
                readPointNode(yshape, "point2", p2);

                qgraph::Rectangle* rect = new qgraph::Rectangle(doc->scene);
                ensureUid(rect);
                rect->setRealSceneRect(QRectF(p1, p2));

                int numberingOffset = 0;

                if (yshape["numbering_offset"])
                    conf->getValue(yshape, "numbering_offset", numberingOffset);

                rect->setNumberingOffset(numberingOffset);

                apply_LineWidth_ToItem(rect);
                apply_PointSize_ToItem(rect);
                apply_NumberSize_ToItem(rect);
                applyClassColorToItem(rect, label);

                created = rect;
            }
            else if (type == "point")
            {
                QPointF center;
                readPointNode(yshape, "point", center);

                qgraph::Point* p = new qgraph::Point(doc->scene);
                ensureUid(p);
                p->setCenter(center);

                apply_PointSize_ToItem(p);
                apply_NumberSize_ToItem(p);
                apply_PointStyle_ToItem(p);
                applyClassColorToItem(p, label);

                created = p;
            }
            else if (type == "line")
            {
                QVector<QPointF> points;
                readPointsArray(yshape, points);

                if (points.isEmpty())
                    continue;

                qgraph::Line* line = new qgraph::Line(doc->scene, points.first());
                ensureUid(line);

                line->beginBulkLoad();
                for (int i = 1; i < points.size(); ++i)
                    line->addPoint(points[i], doc->scene);
                line->endBulkLoad();

                line->closeLine();
                line->updatePath();

                bool numberingFromLast = false;

                if (yshape["numbering_from_last"])
                    conf->getValue(yshape, "numbering_from_last", numberingFromLast);

                line->setNumberingFromLast(numberingFromLast);

                apply_LineWidth_ToItem(line);
                apply_PointSize_ToItem(line);
                apply_NumberSize_ToItem(line);
                applyClassColorToItem(line, label);

                created = line;
            }

            finalizeShape(created, yshape, label);
        }

        raiseAllHandlesToTop();

        return true;
    };

    YamlConfig::Func loadFunc = [&](YamlConfig* conf, YAML::Node& shapes, bool /*logWarn*/)
    {
        if (shapes.IsSequence())
            return loadShapes(conf, shapes, false);

        conf->getValue(shapes, "circles", loadCircles);
        conf->getValue(shapes, "polylines", loadPolylines);
        conf->getValue(shapes, "rectangles", loadRectangles);
        conf->getValue(shapes, "points", loadPoints);
        conf->getValue(shapes, "lines", loadLines);
        return true;
    };

    // Загружаем данные из YAML
    if (!yconfig.getValue("shapes", loadFunc, false))
    {
        log_error_m << "Ошибка при загрузке разметки:" << yamlPath;
        _loadingNow = false;

        return;
    }
    _loadingNow = false;
    if (rebuildUi && doc == currentDocument())
    {
        updatePolygonListForCurrentScene();
    }
}

void MainWindow::saveCurrentViewState(Document::Ptr doc)
{
    if (!doc || !ui->graphView)
    {
        return;
    }

    doc->viewState = {
        ui->graphView->horizontalScrollBar()->value(),
        ui->graphView->verticalScrollBar()->value(),
        ui->graphView->transform().m11(),
        ui->graphView->mapToScene(ui->graphView->viewport()->rect().center())
    };
}

void MainWindow::restoreViewState(Document::Ptr doc)
{
    if (!doc)
    {
        fitImageToView();
        return;
    }

    doc->scene->setSceneRect(doc->videoRect->boundingRect().translated(doc->videoRect->pos()));

    // Если viewState еще не был сохранен
    if (doc->viewState.zoom <= 0.000001)
    {
        fitImageToView();
        return;
    }

    ui->graphView->resetTransform();
    ui->graphView->scale(doc->viewState.zoom, doc->viewState.zoom);
    ui->graphView->horizontalScrollBar()->setValue(doc->viewState.hScroll);
    ui->graphView->verticalScrollBar()->setValue(doc->viewState.vScroll);
    ui->graphView->centerOn(doc->viewState.center);
    _m_zoom = ui->graphView->transform().m11();
    if (_m_zoom <= 0.000001)
        _m_zoom = 1.0;
    updateAllPointNumbers();
    ui->graphView->viewport()->update();
}

void MainWindow::handleCheckBoxClick(QCheckBox* clickedCheckBox)
{
    if (_lastCheckedPolygonLabel == clickedCheckBox)
    {
        clickedCheckBox->setChecked(false);
        _lastCheckedPolygonLabel = nullptr;
    }
    else
    {
        if (_lastCheckedPolygonLabel)
        {
            _lastCheckedPolygonLabel->setChecked(false);
        }
        clickedCheckBox->setChecked(true);
        _lastCheckedPolygonLabel = clickedCheckBox;
    }
}

void MainWindow::loadLastUsedFolder()
{
    if (!config::base().getValue("application.last_used_folder", _lastUsedFolder, false))
    {
        _lastUsedFolder = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    }
    log_verbose_m << "Loaded last used folder: " << _lastUsedFolder;
}

void MainWindow::saveLastUsedFolder()
{
    config::base().setValue("application.last_used_folder", _lastUsedFolder);
    config::base().saveFile();
    log_verbose_m << "Saved last used folder: " << _lastUsedFolder;
}

bool MainWindow::hasAnnotationFile(const QString& imagePath) const
{
    QFileInfo fileInfo(imagePath);
    QString yamlPath = fileInfo.path() + "/" + fileInfo.completeBaseName() + ".yaml";

    if (!QFile::exists(yamlPath)) {
        return false;
    }

    // Проверяем, что файл не пустой
    QFile file(yamlPath);
    if (file.open(QIODevice::ReadOnly))
    {
        return file.size() > 0;
    }
    return false;
}

QString MainWindow::classesYamlPath() const
{
    QDir dir(_currentFolderPath);
    return dir.filePath("classes.yaml");
}

bool MainWindow::loadClassesFromFile(const QString& filePath)
{
    YamlConfig yconfig;
    const QByteArray encoded = QFile::encodeName(QDir::toNativeSeparators(filePath));
    if (!QFile::exists(filePath))
    {
        QFile file;
        QByteArray conf;

        file.setFileName(":/config/resources/classes.base.yaml");
        file.open(QIODevice::ReadOnly);
        conf = file.readAll();

        if (!yconfig.readString(conf.toStdString()))
        {
            messageBox(
                this,
                QMessageBox::Warning,
                u8"Ошибка при чтении базового файла с классами!"
            );
            return false;
        }
        if (!yconfig.saveFile(std::string(encoded.constData(), encoded.size())))
        {
            messageBox(
                this,
                QMessageBox::Warning,
                u8"Ошибка при сохранении в базовый файл с классами!  "
                u8"Более подробную информацию смотри в .log файле"
            );
            return false;
        }
    }
    else
    {
        if (!yconfig.readFile(std::string(encoded.constData(), encoded.size())))
        {
            messageBox(
                this,
                QMessageBox::Warning,
                u8"Ошибка при чтении файла с классами!  "
                u8"Более подробную информацию смотри в .log файле"
            );
            return false;
        }
    }

    QStringList classes;
    QMap<QString, QColor> colors;

    bool newFormatDetected = false;

    YamlConfig::Func loadNewClasses = [&](YamlConfig* conf, YAML::Node& yclasses, bool)
    {
        for (const YAML::Node& yclass : yclasses)
        {
            if (!yclass.IsMap())
                continue;

            QString name;
            conf->getValue(yclass, "name", name);

            if (name.trimmed().isEmpty())
                continue;

            newFormatDetected = true;
            classes.push_back(name);

            if (yclass["color"])
            {
                QString colorStr;
                conf->getValue(yclass, "color", colorStr);

                const QColor c(colorStr);
                if (c.isValid())
                    colors[name] = c;
            }
        }
        return true;
    };

    yconfig.getValue("classes", loadNewClasses, false);

    // TODO
    // Fallback на старый формат, чтобы не сломать уже существующие файлы
    if (!newFormatDetected)
    {
        QList<QString> oldClasses;

        if (!yconfig.getValue("classes", oldClasses, false))
        {
            messageBox(
                this,
                QMessageBox::Warning,
                u8"Нет секции классов"
            );
            return false;
        }

        for (const QString& s : oldClasses)
            classes.push_back(s);

        QList<QString> colorLines;
        if (yconfig.getValue("class_colors", colorLines, true))
        {
            for (const QString& s : colorLines)
            {
                const int eq = s.indexOf('=');
                if (eq <= 0)
                    continue;

                const QString name = s.left(eq).trimmed();
                const QString hex  = s.mid(eq + 1).trimmed();

                const QColor c(hex);
                if (!name.isEmpty() && c.isValid())
                    colors[name] = c;
            }
        }
    }

    bool generatedMissingColors = false;

    std::function<QColor(const QMap<QString, QColor>&, int)> generateRandomClassColor =
        [&](const QMap<QString, QColor>& existingColors, int fallbackIndex) -> QColor
    {
        for (int attempt = 0; attempt < 64; ++attempt)
        {
            const int h = QRandomGenerator::global()->bounded(360);
            QColor c = QColor::fromHsl(h, 200, 120);

            bool alreadyUsed = false;
            for (QMap<QString, QColor>::const_iterator it = existingColors.constBegin();
                 it != existingColors.constEnd(); ++it)
            {
                if (it.value().isValid() && it.value() == c)
                {
                    alreadyUsed = true;
                    break;
                }
            }

            if (!alreadyUsed)
                return c;
        }

        QColor c;
        c.setHsl((fallbackIndex * 47) % 360, 200, 120);
        return c;
    };

    for (int i = 0; i < classes.size(); ++i)
    {
        const QString name = classes[i].trimmed();
        if (name.isEmpty())
            continue;

        const QColor existing = colors.value(name, QColor());
        if (existing.isValid())
            continue;

        colors[name] = generateRandomClassColor(colors, i);
        generatedMissingColors = true;
    }

    _projectClasses = classes;
    _projectClassColors = colors;

    // Cинхронизируем диалог
    if (_projPropsDialog)
    {
        _projPropsDialog->setProjectClasses(_projectClasses);
        _projPropsDialog->setProjectClassColors(_projectClassColors);
    }

    if (generatedMissingColors)
    {
        saveProjectClasses(_projectClasses, _projectClassColors);
    }

    forEachScene([this](QGraphicsScene* sc)
    {
        if (!sc)
            return;

        qgraph::VideoRect* videoRect = nullptr;

        for (QMap<QString, Document::Ptr>::iterator it = _documentsMap.begin();
             it != _documentsMap.end(); ++it)
        {
            Document::Ptr document = it.value();

            if (document && document->scene == sc)
            {
                videoRect = document->videoRect;
                break;
            }
        }

        const QList<QGraphicsItem*> items = sc->items();

        for (QGraphicsItem* item : items)
        {
            if (!item)
                continue;

            if (item == videoRect
                || item == _tempRectItem
                || item == _tempCircleItem
                || item == _tempPolyline)
            {
                continue;
            }

            if (item->parentItem() != nullptr)
                continue;

            const QString cls = item->data(0).toString();

            if (!cls.isEmpty())
                applyClassColorToItem(item, cls);
        }

        sc->update();
    });

    Document::Ptr currentDoc = currentDocument();

    if (currentDoc && currentDoc->scene)
        onSceneSelectionChanged();

    return true;
}

bool MainWindow::saveProjectClasses(const QStringList& classes,
                                    const QMap<QString, QColor>& colors)
{
    const QString path = classesYamlPath();
    if (path.isEmpty())
        return false;

    YamlConfig::Func saveClasses = [&](YamlConfig* conf, YAML::Node& yclasses, bool)
    {
        for (const QString& name : classes)
        {
            YAML::Node yclass;

            conf->setValue(yclass, "name", name);

            const QColor c = colors.value(name, QColor());
            if (c.isValid())
                conf->setValue(yclass, "color", c.name(QColor::HexArgb));

            yclasses.push_back(yclass);
        }
        return true;
    };

    YamlConfig yconfig;
    yconfig.setValue("classes", saveClasses);

    const QByteArray encoded = QFile::encodeName(QDir::toNativeSeparators(path));
    return yconfig.saveFile(std::string(encoded.constData(), encoded.size()));
}

void MainWindow::onPolygonListItemClicked(const QModelIndex& index)
{
    QGraphicsItem* sceneItem = sceneItemFromListIndex(index);

    if (sceneItem)
        ui->graphView->centerOn(sceneItem);
}

void MainWindow::onPolygonListItemDoubleClicked(const QModelIndex& index)
{
    QGraphicsItem* sceneItem = sceneItemFromListIndex(index);

    if (!sceneItem)
        return;

    ui->graphView->centerOn(sceneItem);
    ui->graphView->scale(1.2, 1.2);
}

void MainWindow::onSceneSelectionChanged()
{
    if (_syncingSelection)
        return;

    Document::Ptr doc = currentDocument();

    if (!doc || !doc->scene || !ui || !ui->polygonList)
        return;

    QItemSelectionModel* selectionModel = ui->polygonList->selectionModel();

    if (!selectionModel)
        return;

    _syncingSelection = true;

    QGraphicsScene* scene = doc->scene;
    qgraph::VideoRect* videoRect = doc->videoRect;

    // Обновляем цвета/заливку всех фигур с учетом текущего выделения
    for (QGraphicsItem* item : scene->items())
    {
        if (!item)
            continue;

        if (item == videoRect
            || item == _tempRectItem
            || item == _tempCircleItem
            || item == _tempPolyline)
        {
            continue;
        }

        if (item->parentItem() != nullptr)
            continue;

        const QString className = item->data(0).toString();

        if (!className.isEmpty())
            applyClassColorToItem(item, className);
    }

    selectionModel->clearSelection();
    selectionModel->clearCurrentIndex();

    const QList<QGraphicsItem*> selectedItems = scene->selectedItems();
    bool currentIndexWasSet = false;

    for (QGraphicsItem* sceneItem : selectedItems)
    {
        if (!sceneItem)
            continue;

        if (qgraph::DragCircle* handle = dynamic_cast<qgraph::DragCircle*>(sceneItem))
        {
            if (handle->parentItem())
                sceneItem = handle->parentItem();
        }

        if (sceneItem && sceneItem != videoRect)
            sceneItem = findMovableAncestor(sceneItem);

        if (!sceneItem || sceneItem == videoRect)
            continue;

        const int row = polygonListRowByItem(sceneItem);

        if (row < 0)
            continue;

        const QModelIndex index = doc->polygonList.model.index(row, 0);

        if (!index.isValid())
            continue;

        selectionModel->select(index,
                               QItemSelectionModel::Select
                               | QItemSelectionModel::Rows);

        if (!currentIndexWasSet)
        {
            selectionModel->setCurrentIndex(index, QItemSelectionModel::NoUpdate);
            currentIndexWasSet = true;
        }
    }

    updateCoordinateList();
    raiseSelectedShapesTemporarily();
    scene->update();

    updateShapeListButtons();

    _syncingSelection = false;
}

void MainWindow::updateCoordinateList()
{
    ui->coordinateList->clear();

    Document::Ptr doc = currentDocument();
    if (!doc || !doc->scene)
        return;
    QGraphicsScene* scene = doc->scene;

    if (!scene)
        return;

    QList<QGraphicsItem*> selectedItems = scene->selectedItems();
    if (selectedItems.isEmpty())
        return;

    std::function<void(const QString&)> addLine = [this](const QString& text)
    {
        ui->coordinateList->addItem(text);
    };

    bool (*calcBounds)(const QVector<QPointF>&, QPointF&, QPointF&) =
        [](const QVector<QPointF>& pts, QPointF& topLeft, QPointF& bottomRight) -> bool
    {
        if (pts.isEmpty())
            return false;

        qreal minX = pts[0].x();
        qreal maxX = pts[0].x();
        qreal minY = pts[0].y();
        qreal maxY = pts[0].y();

        for (const QPointF& p : pts)
        {
            if (p.x() < minX) minX = p.x();
            if (p.x() > maxX) maxX = p.x();
            if (p.y() < minY) minY = p.y();
            if (p.y() > maxY) maxY = p.y();
        }

        topLeft = QPointF(minX, minY);
        bottomRight = QPointF(maxX, maxY);
        return true;
    };

    bool firstShape = true;

    for (QGraphicsItem* item : selectedItems)
    {
        if (!item)
            continue;

        if (qgraph::Point* point = dynamic_cast<qgraph::Point*>(item))
        {
            if (!firstShape)
                addLine(QString());
            firstShape = false;

            const QPointF center = point->center();

            addLine(u8"Точка");
            addLine(QString(u8"Центр: (%1; %2)")
                           .arg(round2(center.x()))
                           .arg(round2(center.y())));
        }
        else if (qgraph::Rectangle* rect = dynamic_cast<qgraph::Rectangle*>(item))
        {
            if (!firstShape)
                addLine(QString());
            firstShape = false;

            const QRectF r = rect->realSceneRect();

            const QPointF topLeft = r.topLeft();
            const QPointF bottomRight = r.bottomRight();
            const QPointF center = r.center();

            const qreal width  = r.width();
            const qreal height = r.height();

            addLine(u8"Прямоугольник");
            addLine(QString(u8"TL точка: (%1; %2)")
                           .arg(round2(topLeft.x()))
                           .arg(round2(topLeft.y())));

            addLine(QString(u8"BR точка: (%1; %2)")
                           .arg(round2(bottomRight.x()))
                           .arg(round2(bottomRight.y())));

            addLine(QString(u8"Центр: (%1; %2)")
                           .arg(round2(center.x()))
                           .arg(round2(center.y())));

            addLine(QString(u8"Ширина: %1").arg(round2(width)));
            addLine(QString(u8"Высота: %1").arg(round2(height)));
        }
        else if (qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(item))
        {
            if (!firstShape)
                addLine(QString());
            firstShape = false;

            const QVector<QPointF> pts = polyline->points();

            QPointF topLeft, bottomRight;
            calcBounds(pts, topLeft, bottomRight);

            const QPointF center((topLeft.x() + bottomRight.x()) / 2.0,
                                 (topLeft.y() + bottomRight.y()) / 2.0);

            const qreal width  = bottomRight.x() - topLeft.x();
            const qreal height = bottomRight.y() - topLeft.y();

            addLine(u8"Полилиния");
            addLine(QString(u8"TL точка: (%1; %2)")
                           .arg(round2(topLeft.x()))
                           .arg(round2(topLeft.y())));

            addLine(QString(u8"BR точка: (%1; %2)")
                           .arg(round2(bottomRight.x()))
                           .arg(round2(bottomRight.y())));

            addLine(QString(u8"Центр: (%1; %2)")
                           .arg(round2(center.x()))
                           .arg(round2(center.y())));

            addLine(QString(u8"Ширина: %1").arg(round2(width)));
            addLine(QString(u8"Высота: %1").arg(round2(height)));
        }
        else if (qgraph::Line* line = dynamic_cast<qgraph::Line*>(item))
        {
            if (!firstShape)
                addLine(QString());
            firstShape = false;

            const QVector<QPointF> pts = line->points();

            QPointF topLeft, bottomRight;
            if (!calcBounds(pts, topLeft, bottomRight))
                continue;

            const QPointF center((topLeft.x() + bottomRight.x()) / 2.0,
                                 (topLeft.y() + bottomRight.y()) / 2.0);

            const qreal width  = bottomRight.x() - topLeft.x();
            const qreal height = bottomRight.y() - topLeft.y();

            addLine(u8"Линия");
            addLine(QString(u8"TL точка: (%1; %2)")
                           .arg(round2(topLeft.x()))
                           .arg(round2(topLeft.y())));

            addLine(QString(u8"BR точка: (%1; %2)")
                           .arg(round2(bottomRight.x()))
                           .arg(round2(bottomRight.y())));

            addLine(QString(u8"Центр: (%1; %2)")
                           .arg(round2(center.x()))
                           .arg(round2(center.y())));

            addLine(QString(u8"Ширина: %1").arg(round2(width)));
            addLine(QString(u8"Высота: %1").arg(round2(height)));
        }
        else if (qgraph::Circle* circle = dynamic_cast<qgraph::Circle*>(item))
        {
            if (!firstShape)
                addLine(QString());
            firstShape = false;

            QPointF center = circle->center();
            qreal radius = circle->realRadius();

            QPointF topLeft(center.x() - radius,
                            center.y() - radius);
            QPointF bottomRight(center.x() + radius,
                                center.y() + radius);

            qreal width  = bottomRight.x() - topLeft.x();
            qreal height = bottomRight.y() - topLeft.y();

            addLine(u8"Круг");
            addLine(QString(u8"TL точка: (%1; %2)")
                           .arg(round2(topLeft.x()))
                           .arg(round2(topLeft.y())));

            addLine(QString(u8"BR точка: (%1; %2)")
                           .arg(round2(bottomRight.x()))
                           .arg(round2(bottomRight.y())));

            addLine(QString(u8"Центр: (%1; %2)")
                           .arg(round2(center.x()))
                           .arg(round2(center.y())));

            addLine(QString(u8"Ширина: %1").arg(round2(width)));
            addLine(QString(u8"Высота: %1").arg(round2(height)));
        }
    }
}

void MainWindow::updateFileListItemIcon(QListWidgetItem* item, bool hasAnnotations)
{
    if (!item) return;

    Document::Ptr doc = item->data(Qt::UserRole).value<Document::Ptr>();
    if (!doc) return;

    // Получаем превью изображения
    QPixmap preview(doc->filePath);
    preview = preview.scaled(50, 50, Qt::KeepAspectRatio);

    // Создаем композитное изображение
    QPixmap result(70, 70);
    result.fill(Qt::transparent);

    QPainter painter(&result);
    painter.drawPixmap(10, 10, preview);

    // Добавляем иконку статуса
    QPixmap statusIcon(hasAnnotations ? "/images/resources/ok.svg" : "/images/resources/not_ok.svg");
    statusIcon = statusIcon.scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    painter.drawPixmap(0, 0, statusIcon);

    item->setIcon(QIcon(result));
}

QListWidgetItem* MainWindow::findFileListItem(const QString& filePath)
{
    for (int i = 0; i < ui->fileList->count(); ++i)
    {
        QListWidgetItem* item = ui->fileList->item(i);
        Document::Ptr doc = item->data(Qt::UserRole).value<Document::Ptr>();
        if (doc && doc->filePath == filePath)
        {
            return item;
        }
    }
    return nullptr;
}

void MainWindow::removePolygonItem(QGraphicsItem* item)
{
    if (!item)
        return;

    removeListEntryBySceneItem(item);

    if (item->scene())
        item->scene()->removeItem(item);

    clearLinePolylineStateForDeletedItem(item);
    delete item;

    if (Document::Ptr doc = currentDocument())
    {
        doc->isModified = true;
        updateFileListDisplay(doc->filePath);
    }
}

void MainWindow::removePolygonListRow(int row)
{
    QGraphicsItem* sceneItem = sceneItemFromListRow(row);

    if (!sceneItem)
        return;

    Document::Ptr doc = currentDocument();
    QGraphicsScene* scene = doc->scene;

    if (sceneItem->scene() == scene)
        scene->removeItem(sceneItem);

    clearLinePolylineStateForDeletedItem(sceneItem);
    delete sceneItem;

    if (Document::Ptr doc = currentDocument())
    {
        if (lst::inRange(row, 0, doc->polygonList.items.count()))
        {
            doc->polygonList.items.removeAt(row);
            doc->polygonList.model.removeRow(row);
        }

        renumberPolygonList();

        doc->isModified = true;
        updateFileListDisplay(doc->filePath);
    }
}

void MainWindow::linkSceneItemToList(QGraphicsItem* sceneItem)
{
    linkSceneItemToList(sceneItem, -1, true);
}

void MainWindow::linkSceneItemToList(QGraphicsItem* sceneItem, int row, bool needRenumber)
{
    if (!sceneItem)
        return;

    Document::Ptr doc = currentDocument();
    if (!doc) return;

    QString className = sceneItem->data(0).toString();
    if (className.isEmpty())
        return;

    if (polygonListRowByItem(sceneItem) >= 0)
        return;

    ensureShapeNumber(sceneItem);

    const int count = doc->polygonList.items.size();
    const int insertRow = (row >= 0 && row <= count) ? row : count;

    doc->polygonList.items.insert(insertRow, sceneItem);

    QStandardItem* modelItem = new QStandardItem;
    modelItem->setEditable(false);
    doc->polygonList.model.insertRow(insertRow, modelItem);

    if (needRenumber)
        renumberPolygonList();
    else
        updatePolygonListItemText(insertRow);
}

void MainWindow::renumberPolygonList()
{
    Document::Ptr doc = currentDocument();

    if (!doc)
        return;

    refreshShapeListOrderRole();

    for (int row = 0; row < doc->polygonList.items.size(); ++row)
        updatePolygonListItemText(row);

    updateShapeListButtons();

    if (doc->scene)
        doc->scene->update();
}

void MainWindow::renumberPolygonListTextOnly()
{
    Document::Ptr doc = currentDocument();

    if (!doc)
        return;

    refreshShapeListOrderRole();

    for (int row = 0; row < doc->polygonList.items.size(); ++row)
        updatePolygonListItemText(row);

    updateShapeListButtons();
}

void MainWindow::showPolygonListContextMenu(const QPoint& pos)
{
    if (!ui || !ui->polygonList)
        return;

    QModelIndex index = ui->polygonList->indexAt(pos);

    if (!index.isValid())
        return;

    QGraphicsItem* sceneItem = sceneItemFromListIndex(index);

    if (!sceneItem)
        return;

    QMenu menu;

    QAction* toggleVisibleAction = menu.addAction(
        sceneItem->isVisible() ? u8"Скрыть разметку" : u8"Показать разметку"
    );

    QAction* deleteAction = menu.addAction(u8"Удалить (Del)");

    QAction* chosen = menu.exec(ui->polygonList->viewport()->mapToGlobal(pos));

    if (chosen == toggleVisibleAction)
    {
        toggleSceneItemVisibility(sceneItem);
    }
    else if (chosen == deleteAction)
    {
        QItemSelectionModel* selectionModel = ui->polygonList->selectionModel();

        if (selectionModel && !selectionModel->isSelected(index))
        {
            selectionModel->clearSelection();
            selectionModel->select(index,
                                   QItemSelectionModel::Select | QItemSelectionModel::Rows);
        }

        ui->polygonList->setCurrentIndex(index);
        on_actDelete_triggered();
    }
}

void MainWindow::updatePolygonListItemText(int row)
{
    Document::Ptr doc = currentDocument();
    if (!doc) return;

    if (!lst::inRange(row, 0, doc->polygonList.items.count()))
        return;

    QGraphicsItem* sceneItem = doc->polygonList.items[row];
    if (!sceneItem)
        return;

    QStandardItem* modelItem = doc->polygonList.model.item(row);
    if (!modelItem)
        return;

    int number = ensureShapeNumber(sceneItem);
    QString className = sceneItem->data(0).toString();
    QString text = QString("%1: %2").arg(number).arg(className);

    if (!sceneItem->isVisible())
        text += " [скрыто]";

    modelItem->setText(text);
}

void MainWindow::updatePolygonListTexts()
{
    Document::Ptr doc = currentDocument();

    if (!doc)
        return;

    for (int row = 0; row < doc->polygonList.items.size(); ++row)
        updatePolygonListItemText(row);
}

bool MainWindow::isYamlFileEmpty(const QString& yamlPath) const
{
    QFile file(yamlPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return true; // Если не можем открыть, считаем пустым
    }

    QTextStream in(&file);
    QString content = in.readAll();
    file.close();

    // Убираем комментарии и пустые строки для проверки
    QStringList lines = content.split('\n');
    for (const QString& line : lines)
    {
        QString trimmed = line.trimmed();
        // Пропускаем пустые строки и комментарии
        if (!trimmed.isEmpty() && !trimmed.startsWith('#'))
        {
            // Проверяем наличие ключа shapes (основная структура аннотаций)
            if (trimmed.contains("shapes:") || trimmed.contains("circles:") ||
                trimmed.contains("rectangles:") || trimmed.contains("polylines:") ||
                trimmed.contains("points:") || trimmed.contains("lines:"))
            {
                return false; // Файл не пустой
            }
        }
    }
    return true; // Файл пустой
}

QString MainWindow::getAnnotationFilePath(const QString& imagePath) const
{
    // Преобразуем путь к изображению в путь к YAML файлу
    QFileInfo imageInfo(imagePath);
    QString yamlPath = imageInfo.path() + "/" + imageInfo.completeBaseName() + ".yaml";
    return yamlPath;
}

void MainWindow::raiseAllHandlesToTop()
{
    QList<QGraphicsScene*> scenes;

    Document::Ptr currentDoc = currentDocument();

    if (currentDoc && currentDoc->scene)
    {
        scenes.append(currentDoc->scene);
    }
    else
    {
        for (QMap<QString, Document::Ptr>::iterator it = _documentsMap.begin();
             it != _documentsMap.end(); ++it)
        {
            Document::Ptr doc = it.value();

            if (doc && doc->scene)
                scenes.append(doc->scene);
        }
    }

    for (QGraphicsScene* scene : scenes)
    {
        if (!scene)
            continue;

        const QList<QGraphicsItem*> items = scene->items();

        for (QGraphicsItem* item : items)
        {
            if (qgraph::Rectangle* rectangle = dynamic_cast<qgraph::Rectangle*>(item))
            {
                rectangle->raiseHandlesToTop();
            }
            else if (qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(item))
            {
                polyline->raiseHandlesToTop();
            }
            else if (qgraph::Circle* circle = dynamic_cast<qgraph::Circle*>(item))
            {
                circle->raiseHandlesToTop();
            }
            else if (qgraph::Point* point = dynamic_cast<qgraph::Point*>(item))
            {
                point->raiseHandlesToTop();
            }
        }
    }
}

bool MainWindow::isRootShapeItem(QGraphicsItem* item) const
{
    if (!item)
        return false;

    Document::Ptr doc = currentDocument();
    qgraph::VideoRect* videoRect = doc ? doc->videoRect : nullptr;

    if (item == videoRect ||
        item == _tempRectItem ||
        item == _tempCircleItem ||
        item == _tempPolyline)
    {
        return false;
    }

    if (item->parentItem() != nullptr)
        return false;

    return dynamic_cast<qgraph::Rectangle*>(item) ||
           dynamic_cast<qgraph::Circle*>(item) ||
           dynamic_cast<qgraph::Polyline*>(item) ||
           dynamic_cast<qgraph::Line*>(item) ||
           dynamic_cast<qgraph::Point*>(item);
}

qreal MainWindow::originalZValueForItem(QGraphicsItem* item) const
{
    if (!item)
        return 0.0;

    const qulonglong uid = item->data(_roleUid).toULongLong();
    if (uid != 0 && _temporaryRaisedZValues.contains(uid))
        return _temporaryRaisedZValues.value(uid);

    return item->zValue();
}

void MainWindow::restoreTemporaryRaisedZValues()
{
    if (_temporaryRaisedZValues.isEmpty())
        return;

    const QHash<qulonglong, qreal> raised = _temporaryRaisedZValues;
    _temporaryRaisedZValues.clear();

    for (QHash<qulonglong, qreal>::const_iterator it = raised.constBegin();
         it != raised.constEnd(); ++it)
    {
        QGraphicsItem* item = findItemByUid(it.key());
        if (!item)
            continue;

        item->setZValue(it.value());
    }
    Document::Ptr doc = currentDocument();

    if (doc && doc->scene)
        doc->scene->update();
}

void MainWindow::raiseSelectedShapesTemporarily()
{
    Document::Ptr doc = currentDocument();
    if (!doc || !doc->scene)
        return;

    QGraphicsScene* scene = doc->scene;
    qgraph::VideoRect* videoRect = doc->videoRect;

    restoreTemporaryRaisedZValues();

    QList<QGraphicsItem*> selectedRoots;

    qreal maxZ = videoRect ? videoRect->zValue() : 0.0;

    for (QGraphicsItem* item : scene->items())
    {
        if (!isRootShapeItem(item))
            continue;

        maxZ = std::max(maxZ, item->zValue());

        if (item->isSelected())
            selectedRoots.append(item);
    }

    if (selectedRoots.isEmpty())
        return;

    qreal z = maxZ + 1.0;

    for (QGraphicsItem* item : selectedRoots)
    {
        if (!item)
            continue;

        const qulonglong uid = ensureUid(item);
        if (uid == 0)
            continue;

        _temporaryRaisedZValues.insert(uid, item->zValue());
        item->setZValue(z++);
    }

    raiseAllHandlesToTop();

    if (scene)
        scene->update();
}

void MainWindow::moveSelectedItemsToBack()
{
    Document::Ptr doc = currentDocument();
    QGraphicsScene* scene = doc->scene;
    qgraph::VideoRect* videoRect = doc->videoRect;

    if (!scene)
        return;

    restoreTemporaryRaisedZValues();

    QList<QGraphicsItem*> selectedRoots;
    QList<QGraphicsItem*> nonSelectedRoots;

    std::function<bool(QGraphicsItem*)> isRootShape = [this](QGraphicsItem* item) -> bool
    {
        if (!item)
            return false;

        Document::Ptr doc = currentDocument();
        qgraph::VideoRect* videoRect = doc->videoRect;

        if (item == videoRect ||
            item == _tempRectItem ||
            item == _tempCircleItem ||
            item == _tempPolyline)
        {
            return false;
        }

        if (item->parentItem() != nullptr)
            return false;

        return dynamic_cast<qgraph::Rectangle*>(item) ||
               dynamic_cast<qgraph::Circle*>(item) ||
               dynamic_cast<qgraph::Polyline*>(item) ||
               dynamic_cast<qgraph::Line*>(item) ||
               dynamic_cast<qgraph::Point*>(item);
    };

    for (QGraphicsItem* item : scene->items())
    {
        if (!isRootShape(item))
            continue;

        if (item->isSelected())
            selectedRoots.append(item);
        else
            nonSelectedRoots.append(item);
    }

    if (selectedRoots.isEmpty())
        return;

    bool (*byZ)(QGraphicsItem*, QGraphicsItem*) =
        [](QGraphicsItem* firstItem, QGraphicsItem* secondItem)
    {
        if (qFuzzyCompare(firstItem->zValue(), secondItem->zValue()))
            return firstItem < secondItem;

        return firstItem->zValue() < secondItem->zValue();
    };

    std::sort(selectedRoots.begin(), selectedRoots.end(), byZ);
    std::sort(nonSelectedRoots.begin(), nonSelectedRoots.end(), byZ);

    QList<QGraphicsItem*> orderedRoots;
    orderedRoots.reserve(selectedRoots.size() + nonSelectedRoots.size());

    // Выбранные фигуры уходят в самый низ среди фигур
    // Их внутренний относительный порядок сохраняется
    orderedRoots.append(selectedRoots);
    orderedRoots.append(nonSelectedRoots);

    const qreal imageZ = (videoRect ? videoRect->zValue() : 0.0);
    qreal z = imageZ + 1.0;

    for (QGraphicsItem* item : orderedRoots)
    {
        if (item)
            item->setZValue(z++);
    }

    {
        QSignalBlocker blockScene(scene);
        QSignalBlocker blockList(ui->polygonList);

        for (QGraphicsItem* item : selectedRoots)
        {
            if (item)
                item->setSelected(false);
        }

        if (ui && ui->polygonList)
            ui->polygonList->clearSelection();
    }

    if (ui && ui->coordinateList)
        ui->coordinateList->clear();

    raiseAllHandlesToTop();

    if (scene)
        scene->update();
}
bool MainWindow::hasUnsavedChanges() const
{
    for (const Document::Ptr& doc : _documentsMap.values())
    {
        if (doc && doc->isModified)
            return true;
    }
    return false;
}

QList<Document::Ptr> MainWindow::getUnsavedDocuments() const
{
    QList<Document::Ptr> unsavedDocs;
    for (QMap<QString, Document::Ptr>::const_iterator it = _documentsMap.constBegin();
         it != _documentsMap.constEnd(); ++it)
    {
        if (it.value()->isModified)
        {
            unsavedDocs.append(it.value());
        }
    }
    return unsavedDocs;
}

int MainWindow::showUnsavedChangesDialog(const QList<Document::Ptr>& unsavedDocs)
{
    QStringList documents;

    for (const Document::Ptr& doc : unsavedDocs)
    {
        if (!doc)
            continue;

        QFileInfo fileInfo {doc->filePath};
        documents.append(fileInfo.fileName());
    }

    UnsavedChanges dialog {documents, this};

    dialog.loadGeometry();

    int res = dialog.exec();
    dialog.saveGeometry();

    if (res != QDialog::Accepted)
        return QDialogButtonBox::Cancel;

    return dialog.selectedButton();
}

void MainWindow::saveAllDocuments()
{
    for (QMap<QString, Document::Ptr>::const_iterator it = _documentsMap.begin();
         it != _documentsMap.end(); ++it)
    {
        if (it.value()->isModified)
        {
            saveAnnotationToFile(it.value());
        }
    }
}

qgraph::DragCircle* MainWindow::pickHiddenHandle(const QPointF& scenePos, bool& topIsHandle) const
{
    Document::Ptr doc = currentDocument();
    QGraphicsScene* scene = doc->scene;

    topIsHandle = false;
    if (!scene) return nullptr;

    GraphicsView* view = ui ? ui->graphView : nullptr;
    const qreal radiusScene = viewPixelsToSceneRadius(view, effectiveViewPickPx(_ghostPickRadius));
    if (radiusScene <= 0.0)
      return nullptr;

    const QRectF pickRect(scenePos - QPointF(radiusScene, radiusScene),
                        QSizeF(2 * radiusScene, 2 * radiusScene));

    const QList<QGraphicsItem*> items = scene->items(pickRect, Qt::IntersectsItemShape, Qt::DescendingOrder);

    // Если верхний элемент уже ручка - «обманка» не нужна, пусть работает стандартный drag
    if (!items.isEmpty())
        if (qgraph::DragCircle* handle = dynamic_cast<qgraph::DragCircle*>(items.front()))
        {
            topIsHandle = true;
            return handle;
        }

    // Ищем ближайшую ручку в глубине стека
    qgraph::DragCircle* best = nullptr;
    qreal bestD2 = std::numeric_limits<qreal>::max();

    for (QGraphicsItem* it : items)
    {
        if (qgraph::Circle* circle = dynamic_cast<qgraph::Circle*>(it))
        {
            QPointF localCursorPos = circle->mapFromScene(scenePos);
            if (circle->isCursorNearCircle(localCursorPos))
            {
                qgraph::DragCircle* handle = circle->getDragCircle();
                if (handle && handle->isValid())
                {
                    const QPointF c = handle->mapToScene(handle->rect().center());
                    const qreal dx = c.x() - scenePos.x();
                    const qreal dy = c.y() - scenePos.y();
                    const qreal d2 = dx*dx + dy*dy;
                    if (d2 <= bestD2)
                    {
                        bestD2 = d2;
                        best = handle;
                    }
                }
            }
            continue;
        }
        if (qgraph::DragCircle* handle = dynamic_cast<qgraph::DragCircle*>(it))
        {
            const QPointF c = handle->mapToScene(handle->rect().center());
            const qreal dx = c.x() - scenePos.x();
            const qreal dy = c.y() - scenePos.y();
            const qreal d2 = dx*dx + dy*dy;
            if (d2 <= bestD2)
            {
                bestD2 = d2;
                best = handle;
            }
        }
    }
    return best;
}

void MainWindow::ensureGhostAllocated()
{
    Document::Ptr doc = currentDocument();

    if (!doc || !doc->scene)
        return;

    QGraphicsScene* scene = doc->scene;

    if (!_ghostHandle)
    {
        _ghostHandle = new QGraphicsRectItem();
    }

    if (_ghostHandle->scene() != scene)
    {
        if (_ghostHandle->scene())
            _ghostHandle->scene()->removeItem(_ghostHandle);
        if (scene)
            scene->addItem(_ghostHandle);
    }

    // Всегда (пере)настраиваем, чтобы гарантировать корректный Z и поведение
    _ghostHandle->setAcceptHoverEvents(true);
    _ghostHandle->setAcceptedMouseButtons(Qt::NoButton);
    _ghostHandle->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);

    // Держим «призрака» поверх всего
    _ghostHandle->setZValue(1e9);
    _ghostHandle->hide();
}

void MainWindow::showGhostOver(qgraph::DragCircle* target, const QPointF& scenePos)
{
    ensureGhostAllocated();
    _ghostTarget = target;
    _ghostActive = true;

    QColor ghostColor = _vstyle.selectedHandleColor;
    _ghostHandle->setRect(target->baseRect());
    _ghostHandle->setPen (target->basePen());
    _ghostHandle->setBrush(QBrush(ghostColor));
    DragCircle::rememberCurrentAsBase(_ghostHandle);

    // Поставим «призрака» поверх позиции настоящей ручки
    const QPointF sceneHandlePos = target->scenePos();
    _ghostHandle->setPos(sceneHandlePos);
    setGhostStyleHover(), _ghostHover = true;
    _ghostHandle->show();

    _ghostGrabOffset = QPointF(0,0);
    _ghostTarget->setGhostDriven(true);
}

void MainWindow::moveGhostTo(const QPointF& scenePos)
{
    if (!_ghostActive || !_ghostTarget) return;

    // Двигаем «призрака» центром под курсор
    _ghostHandle->setPos(scenePos - _ghostHandle->rect().center());

    QGraphicsItem* parent = _ghostTarget->parentItem();
    const QPointF sceneTopLeft = scenePos - _ghostTarget->rect().center();
    const QPointF newLocalTopLeft = parent ? parent->mapFromScene(sceneTopLeft) : sceneTopLeft;

    if (_ghostHandle && _ghostHandle->isVisible())
    {
        if (_ghostActive)
            setGhostStyleHover();
    }
    _ghostTarget->setPos(newLocalTopLeft); // это триггерит itemChange() -> dragCircleMove()
    _handleDragHadChanges = true;
    updateCoordinateList();
}

void MainWindow::endGhost()
{
    if (_ghostTarget)
    {
        _ghostTarget->setHoverStyle(false);
    }
    if (_lastHoverHandle)
    {
        _lastHoverHandle->setHoverStyle(false);
        _lastHoverHandle = nullptr;
    }

    if (_ghostTarget)
    {
        if (qgraph::Shape* shape = dynamic_cast<qgraph::Shape*>(_ghostTarget->parentItem()))
            shape->dragCircleRelease(_ghostTarget);

        pushHandleEditCommandToStack();

        _ghostTarget->setHoverStyle(false);
        _ghostTarget->setGhostDriven(false);

        // Помечаем документ как измененный после завершения перетаскивания
        if (_handleDragHadChanges)
        {
            Document::Ptr doc = currentDocument();

            if (doc && !doc->isModified)
            {
                doc->isModified = true;
                updateFileListDisplay(doc->filePath);
            }
        }
    }

    _handleEditedItem = nullptr;
    _handleEditedIndex = -1;
    _handlePointBefore = QPointF();
    _handleRectBefore = QRectF();
    _handleCircleRadiusBefore = 0;
    _handleDragHadChanges = false;

    _ghostActive = false;
    _ghostHover  = false;
    _ghostTarget = nullptr;

    _handleDragging = false; // Завершение перетаскивания
    updateModeLabel();
    if (_ghostHandle)
    {
        _ghostHandle->hide();
        setGhostStyleIdle();
    }
    updateAllPointNumbers();
}

void MainWindow::setGhostStyleHover()
{
    if (!_ghostHandle || !_ghostTarget)
        return;

    const QPointF centerScene = _ghostHandle->mapToScene(QPointF(0,0));

    // Увеличим на 1.5x относительно базового размера реальной ручки
    const QRectF base = _ghostTarget->baseRect();
    const qreal scale = 1.5;
    const QSizeF newSize(base.size() * scale);
    const QRectF newRect(QPointF(-newSize.width()/2.0, -newSize.height()/2.0), newSize);

    QColor ghostColor = _vstyle.selectedHandleColor;
    _ghostHandle->setRect(newRect);
    _ghostHandle->setPen(QPen(Qt::black, 1));
    _ghostHandle->setBrush(QBrush(ghostColor));
    _ghostHandle->update();

    _ghostHandle->setPos(centerScene - newRect.center());
}

void MainWindow::setGhostStyleIdle()
{
    if (!_ghostHandle || !_ghostTarget) return;

    const QPointF centerScene = _ghostHandle->mapToScene(_ghostHandle->rect().center());

    qreal _smallSize = 8.0;
    _ghostHandle->setRect(-_smallSize/2, -_smallSize/2, _smallSize, _smallSize);
    QColor color(255, 255, 255, 200);
    _ghostHandle->setBrush(QBrush(color));
    _ghostHandle->setPen(QPen(Qt::black, 1));
    _ghostHandle->setPen(QPen(color.darker(200)));

    _ghostHandle->setPos(centerScene - _ghostHandle->rect().center());
    _ghostHandle->update();
}

void MainWindow::showGhostPreview(qgraph::DragCircle* target, const QPointF& scenePos)
{
    ensureGhostAllocated();
    _ghostTarget = target;

    _ghostHandle->setRect(target->baseRect());
    _ghostHandle->setPen(target->basePen());
    _ghostHandle->setBrush(target->baseBrush());
    DragCircle::rememberCurrentAsBase(_ghostHandle);
    _ghostHandle->setPos(target->scenePos());
    setGhostStyleHover(), _ghostHover = true;

    _ghostHandle->show();
}

void MainWindow::startGhostDrag(const QPointF& scenePos)
{
    if (!_ghostTarget || !_ghostHandle)
        return;

    _ghostActive = true;
    _ghostHover  = false;

    _ghostGrabOffset = scenePos - _ghostHandle->pos();
    _ghostTarget->setGhostDriven(true);

    startHandleEditState(_ghostTarget);

    _handleDragging = true;
    updateModeLabel();

    setGhostStyleHover();
}

qreal MainWindow::imagePickScale() const
{
    return 1.0;
}

qreal MainWindow::viewPixelsToSceneRadius(GraphicsView* view, qreal viewPx) const
{
    if (!view || viewPx <= 0.0)
        return 0.0;

    const QTransform t = view->viewportTransform();

    const qreal sx = std::hypot(t.m11(), t.m21());
    const qreal sy = std::hypot(t.m22(), t.m12());
    const qreal s  = std::max<qreal>(sx, sy);

    if (s <= 0.000001)
        return viewPx;

    return viewPx / s;
}

qreal MainWindow::effectiveViewPickPx(qreal baseValue) const
{
    return baseValue* imagePickScale();
}

qgraph::DragCircle* MainWindow::pickHandleAt(const QPointF& scenePos) const
{
    return pickHandleAt(scenePos, _ghostPickRadius);
}

qgraph::DragCircle* MainWindow::pickHandleAt(const QPointF& scenePos, qreal customRadius) const
{
    Document::Ptr doc = currentDocument();
    QGraphicsScene* scene = doc->scene;

    if (!scene)
        return nullptr;

    GraphicsView* view = ui ? ui->graphView : nullptr;
    const qreal pickRadius = viewPixelsToSceneRadius(view, effectiveViewPickPx(customRadius));
    if (pickRadius <= 0.0)
        return nullptr;

    const QRectF probe(scenePos - QPointF(pickRadius, pickRadius),
                       QSizeF(pickRadius * 2, pickRadius * 2));

    qgraph::DragCircle* best = nullptr;
    qreal bestD2 = std::numeric_limits<qreal>::max();

    for (QGraphicsItem* item : scene->items(probe, Qt::IntersectsItemShape))
    {
        if (!item || !item->scene())
            continue;

        if (qgraph::Circle* circle = dynamic_cast<qgraph::Circle*>(item))
        {
            if (circle->isCursorNearCircle(circle->mapFromScene(scenePos)))
            {
                qgraph::DragCircle* handle = circle->getDragCircle();
                if (handle && handle->isValid())
                {
                    circle->updateHandlePosition(scenePos);
                    return handle;
                }
            }
            continue;
        }

        if (qgraph::DragCircle* handle = dynamic_cast<qgraph::DragCircle*>(item))
        {
            if (!handle->isValid())
                continue;

            const QPointF c = handle->mapToScene(handle->rect().center());
            const qreal dx = c.x() - scenePos.x();
            const qreal dy = c.y() - scenePos.y();
            const qreal d2 = dx * dx + dy * dy;
            const qreal r2 = pickRadius * pickRadius;

            if (d2 > r2)
                continue;

            if (d2 < bestD2)
            {
                bestD2 = d2;
                best = handle;
            }
        }
    }
    return best;
}

QGraphicsItem* MainWindow::pickItemByEdgeAt(GraphicsView* view, const QPoint& viewPos) const
{
    if (!view)
        return nullptr;

    QGraphicsScene* sc = view->scene();
    if (!sc)
        return nullptr;

    const QPointF scenePos = view->mapToScene(viewPos);

    const qreal radiusScene = viewPixelsToSceneRadius(view, effectiveViewPickPx(_edgePickRadius));

    if (radiusScene <= 0.0)
        return nullptr;

    const QRectF probe(scenePos - QPointF(radiusScene, radiusScene),
                       QSizeF(radiusScene * 2.0, radiusScene * 2.0));

    Document::Ptr doc = currentDocument();
    QGraphicsScene* scene = doc->scene;
    qgraph::VideoRect* videoRect = doc->videoRect;

    const QList<QGraphicsItem*> items =
        scene->items(probe, Qt::IntersectsItemBoundingRect, Qt::DescendingOrder);

    for (QGraphicsItem* item : items)
    {
        if (!item || !item->isVisible() || !item->scene())
            continue;

        if (item == videoRect)
            continue;

        if (dynamic_cast<qgraph::DragCircle*>(item))
            continue;

        // Берем только фигуры
        const bool isShape =
                (dynamic_cast<qgraph::Polyline*>(item)     != nullptr)
                || (dynamic_cast<qgraph::Line*>(item)      != nullptr)
                || (dynamic_cast<qgraph::Rectangle*>(item) != nullptr)
                || (dynamic_cast<qgraph::Circle*>(item)    != nullptr);

        if (!isShape)
            continue;

        const QPointF pLocal  = item->mapFromScene(scenePos);
        const QPointF pLocal2 = item->mapFromScene(scenePos + QPointF(radiusScene, 0));
        const qreal radiusLocal = QLineF(pLocal, pLocal2).length();
        if (radiusLocal <= 0.0)
            continue;

        QPainterPathStroker stroker;
        stroker.setWidth(2.0 * radiusLocal);
        stroker.setCapStyle(Qt::RoundCap);
        stroker.setJoinStyle(Qt::RoundJoin);

        const QPainterPath hit = stroker.createStroke(item->shape());
        if (hit.contains(pLocal))
            return item;
    }

    return nullptr;
}

void MainWindow::startHandleEditState(qgraph::DragCircle* handle)
{
    // Запоминаем владельца ручки и минимальное состояние до редактирования
    _handleEditedItem = handle ? handle->parentItem() : nullptr;
    _handleEditedIndex = -1;
    _handlePointBefore = QPointF();
    _handleRectBefore = QRectF();
    _handleCircleRadiusBefore = 0;
    _handleDragHadChanges = false;

    if (!_handleEditedItem)
        return;

    if (qgraph::Point* point = dynamic_cast<qgraph::Point*>(_handleEditedItem))
    {
        // Для точки достаточно запомнить ее центр
        _handlePointBefore = point->center();
    }
    else if (qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(_handleEditedItem))
    {
        _handleEditedIndex = polyline->circles().indexOf(handle);

        const QVector<QPointF> points = polyline->points();

        if (_handleEditedIndex >= 0 && _handleEditedIndex < points.size())
            _handlePointBefore = points[_handleEditedIndex];
    }
    else if (qgraph::Line* line = dynamic_cast<qgraph::Line*>(_handleEditedItem))
    {
        _handleEditedIndex = line->circles().indexOf(handle);

        const QVector<QPointF> points = line->points();

        if (_handleEditedIndex >= 0 && _handleEditedIndex < points.size())
            _handlePointBefore = points[_handleEditedIndex];
    }
    else if (qgraph::Rectangle* rectangle = dynamic_cast<qgraph::Rectangle*>(_handleEditedItem))
    {
        _handleRectBefore = rectangle->mapRectToScene(rectangle->rect());
    }
    else if (qgraph::Circle* circle = dynamic_cast<qgraph::Circle*>(_handleEditedItem))
    {
        _handleCircleRadiusBefore = circle->realRadius();
    }
}

void MainWindow::pushHandleEditCommandToStack()
{
    // Если перетаскивания не было или фигура не определена, команду в стек не добавляем
    if (!_handleEditedItem || !_handleDragHadChanges)
        return;

    undo::Edit::Data data;
    QString description;
    bool hasEditData = false;

    if (qgraph::Point* point = dynamic_cast<qgraph::Point*>(_handleEditedItem))
    {
        const QPointF delta = point->center() - _handlePointBefore;

        if (!qFuzzyIsNull(delta.x()) || !qFuzzyIsNull(delta.y()))
        {
            data.type = undo::Edit::Type::MovePoint;
            data.delta = delta;
            hasEditData = true;
            description = u8"Перемещение точки";
        }
    }
    else if (qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(_handleEditedItem))
    {
        // Для полилинии сохраняем только индекс узла и смещение этого узла
        const QVector<QPointF> points = polyline->points();

        if (_handleEditedIndex >= 0 && _handleEditedIndex < points.size())
        {
            const QPointF delta = points[_handleEditedIndex] - _handlePointBefore;

            if (!qFuzzyIsNull(delta.x()) || !qFuzzyIsNull(delta.y()))
            {
                data.type = undo::Edit::Type::MovePolylineNode;
                data.pointIndex = _handleEditedIndex;
                data.delta = delta;
                hasEditData = true;
                description = u8"Перемещение узла полилинии";
            }
        }
    }
    else if (qgraph::Line* line = dynamic_cast<qgraph::Line*>(_handleEditedItem))
    {
        const QVector<QPointF> points = line->points();

        if (_handleEditedIndex >= 0 && _handleEditedIndex < points.size())
        {
            const QPointF delta = points[_handleEditedIndex] - _handlePointBefore;

            if (!qFuzzyIsNull(delta.x()) || !qFuzzyIsNull(delta.y()))
            {
                data.type = undo::Edit::Type::MoveLineNode;
                data.pointIndex = _handleEditedIndex;
                data.delta = delta;
                hasEditData = true;
                description = u8"Перемещение узла линии";
            }
        }
    }
    else if (qgraph::Rectangle* rectangle = dynamic_cast<qgraph::Rectangle*>(_handleEditedItem))
    {
        const QRectF rectAfter = rectangle->mapRectToScene(rectangle->rect());

        if (_handleRectBefore != rectAfter)
        {
            data.type = undo::Edit::Type::EditRectangle;
            data.rectBefore = _handleRectBefore;
            data.rectAfter = rectAfter;
            hasEditData = true;
            description = u8"Редактирование прямоугольника";
        }
    }
    else if (qgraph::Circle* circle = dynamic_cast<qgraph::Circle*>(_handleEditedItem))
    {
        const qreal radiusAfter = circle->realRadius();

        if (!qFuzzyIsNull(radiusAfter - _handleCircleRadiusBefore))
        {
            data.type = undo::Edit::Type::EditCircle;
            data.circleRadiusBefore = _handleCircleRadiusBefore;
            data.circleRadiusAfter = radiusAfter;
            hasEditData = true;
            description = u8"Изменение радиуса круга";
        }
    }

    if (!hasEditData)
        return;

    if (description.isEmpty())
        description = u8"Редактирование фигуры";

    if (Document::Ptr doc = currentDocument())
    {
        if (QUndoStack* stack = activeUndoStack())
            stack->push(new undo::Edit(doc.get(), _handleEditedItem, data, description));
    }
}

void MainWindow::startHandleDrag(qgraph::DragCircle* handle, const QPointF& scenePos)
{    
    _m_isDraggingHandle = true;
    _m_dragHandle = handle;
    _handleDragging = true; // Начало перетаскивания ручки
    updateModeLabel();

    startHandleEditState(handle);

    const QPointF handleCenter = handle->sceneBoundingRect().center();
    _m_pressLocalOffset = scenePos - handleCenter;
    handle->setHoverStyle(true);
}

void MainWindow::updateHandleDrag(const QPointF& scenePos)
{
    if (!_m_dragHandle) return;

    QGraphicsItem* parent = _m_dragHandle->parentItem();
    QPointF parentPos = parent
                            ? parent->mapFromScene(scenePos - _m_pressLocalOffset)
                            : (scenePos - _m_pressLocalOffset);

    _m_dragHandle->setPos(parentPos);

    emit _m_dragHandle->dragCircleMove(_m_dragHandle->index(),
                                       _m_dragHandle->scenePos());
    updateAllPointNumbers();
    _handleDragHadChanges = true;
    updateCoordinateList();
}

void MainWindow::finishHandleDrag()
{
    if (_m_dragHandle)
    {
        emit _m_dragHandle->dragCircleRelease(_m_dragHandle->index(),
                                             _m_dragHandle->scenePos());

        // Помечаем документ как измененный после завершения перетаскивания
        if (_handleDragHadChanges)
        {
            Document::Ptr doc = currentDocument();

            if (doc && !doc->isModified)
            {
                doc->isModified = true;
                updateFileListDisplay(doc->filePath);
            }
        }
        _m_dragHandle->restoreBaseStyle();
    }

    pushHandleEditCommandToStack();

    _handleEditedItem = nullptr;
    _handleEditedIndex = -1;
    _handlePointBefore = QPointF();
    _handleRectBefore = QRectF();
    _handleCircleRadiusBefore = 0;
    _handleDragHadChanges = false;

    _m_isDraggingHandle = false;
    _m_dragHandle = nullptr;
    _handleDragging = false; // Завершение перетаскивания ручки
    updateAllPointNumbers();
    updateModeLabel();
}

void MainWindow::clearAllHandleHoverEffects()
{
    if (_currentHoveredHandle && _currentHoveredHandle->isValid())
    {
         _currentHoveredHandle->setHoverStyle(false);
    }
    _currentHoveredHandle = nullptr;
    hideGhost();
}

void MainWindow::showGhostFor(qgraph::DragCircle* handle)
{
    if (!handle) return;
    ensureGhostAllocated();

    _ghostTarget = handle;
    _ghostHover  = true;
    _ghostActive = false;

    // Для круга используем специальную логику позиционирования
    if (qgraph::Circle* circle = dynamic_cast<qgraph::Circle*>(handle->parentItem()))
    {
        // Получаем позицию курсора в сцене
        QPointF cursorPos = ui->graphView->mapToScene(ui->graphView->mapFromGlobal(QCursor::pos()));

        // Проверяем, что курсор действительно рядом с окружностью
        QPointF localCursorPos = circle->mapFromScene(cursorPos);
        if (circle->isCursorNearCircle(localCursorPos))
        {
            // Обновляем позицию реальной ручки
            circle->updateHandlePosition(cursorPos);

            const QPointF sceneHandlePos = handle->scenePos();
            _ghostHandle->setPos(sceneHandlePos);
            _ghostHandle->setVisible(true);
        }
        else
        {
            // Курсор не рядом с окружностью - не показываем призрак
            hideGhost();
            return;
        }
    }
    else
    {
        // Для других фигур используем старую логику
        const QPointF centerScene = handle->sceneBoundingRect().center();
        _ghostHandle->setPos(centerScene - _ghostHandle->rect().center());
    }

    const QPointF centerScene = handle->sceneBoundingRect().center();
    const QRectF base = _ghostTarget->baseRect();
    const qreal scale = 1.5;
    const QSizeF newSize(base.size() * scale);
    const QRectF newRect(QPointF(-newSize.width()/2.0, -newSize.height()/2.0), newSize);

    QColor ghostColor = _vstyle.selectedHandleColor;
    _ghostHandle->setRect(newRect);
    _ghostHandle->setPen(QPen(Qt::black, 1));
    _ghostHandle->setBrush(QBrush(ghostColor));
    _ghostHandle->setPos(centerScene - newRect.center());
    _ghostHandle->setVisible(true);
    _ghostHandle->update();
}

void MainWindow::hideGhost()
{
    // Восстанавливаем оригинальный стиль реальной ручки, если она была подсвечена
    if (_ghostTarget)
    {
        _ghostTarget->setHoverStyle(false);
    }
    _ghostTarget = nullptr;
    if (_ghostHandle)
    {
        _ghostHandle->setVisible(false);
    }
}

void MainWindow::updateAllPointNumbers()
{
    Document::Ptr doc = currentDocument();
    QGraphicsScene* scene = doc->scene;

    if (!scene) return;

    for (QGraphicsItem* item : scene->items())
    {
        if (qgraph::Rectangle* rectangle = dynamic_cast<qgraph::Rectangle*>(item))
        {
            rectangle->updatePointNumbers();
        }
        else if (qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(item))
        {
            polyline->updatePointNumbers();
        }
        else if (qgraph::Line* line = dynamic_cast<qgraph::Line*>(item))
        {
            line->updatePointNumbers();
        }
    }
}

void MainWindow::loadVisualStyle()
{
    // Устанавливаем значения по умолчанию, если они не найдены в конфиге
    if (!config::base().getValue("graphics.line_width", _vstyle.lineWidth))
        _vstyle.lineWidth = 2.0;

    if (!config::base().getValue("graphics.handle_size", _vstyle.handleSize))
        _vstyle.handleSize = 10.0;

    if (!config::base().getValue("graphics.handle_pick_radius", _ghostPickRadius))
        _ghostPickRadius = 6.0;

    if (!config::base().getValue("graphics.edge_pick_radius", _edgePickRadius))
        _edgePickRadius = 8.0;

    if (!config::base().getValue("graphics.number_font_pt", _vstyle.numberFontPt))
        _vstyle.numberFontPt = 10.0;

    if (!config::base().getValue("graphics.show_numbers", _vstyle.showNumbers))
        _vstyle.showNumbers = false;

    if (!config::base().getValue("graphics.show_selection_frame", _vstyle.showSelectionFrame))
        _vstyle.showSelectionFrame = false;

    if (!config::base().getValue("graphics.fill_shape_when_selected", _vstyle.fillShapeWhenSelected))
        _vstyle.fillShapeWhenSelected = true;

    if (!config::base().getValue("graphics.point_size", _vstyle.pointSize))
        _vstyle.pointSize = 6;

    // Загружаем цвета
    QString colorStr;
    if (config::base().getValue("graphics.handle_color", colorStr))
        _vstyle.handleColor = QColor(colorStr);
    else
        _vstyle.handleColor = Qt::red;

    if (config::base().getValue("graphics.number_color", colorStr))
        _vstyle.numberColor = QColor(colorStr);
    else
        _vstyle.numberColor = Qt::white;

    if (config::base().getValue("graphics.number_bg_color", colorStr))
        _vstyle.numberBgColor = QColor(colorStr);
    else
        _vstyle.numberBgColor = QColor(0, 0, 0, 180);

    // Загружаем цвета линий
    if (config::base().getValue("graphics.colors.rectangle", colorStr))
        _vstyle.rectangleLineColor = QColor(colorStr);
    else
        _vstyle.rectangleLineColor = Qt::green;

    if (config::base().getValue("graphics.colors.circle", colorStr))
        _vstyle.circleLineColor = QColor(colorStr);
    else
        _vstyle.circleLineColor = Qt::red;

    if (config::base().getValue("graphics.colors.polyline", colorStr))
        _vstyle.polylineLineColor = QColor(colorStr);
    else
        _vstyle.polylineLineColor = Qt::blue;

    if (config::base().getValue("graphics.all.selected_handle_color", colorStr))
        _vstyle.selectedHandleColor = QColor(colorStr);
    else
        _vstyle.selectedHandleColor = Qt::yellow;

    if (config::base().getValue("graphics.colors.line", colorStr))
        _vstyle.lineLineColor = QColor(colorStr);

    if (config::base().getValue("graphics.colors.point", colorStr))
        _vstyle.pointColor = QColor(colorStr);

    if (config::base().getValue("graphics.colors.ruler", colorStr))
        _vstyle.rulerColor = QColor(colorStr);
    else
        _vstyle.rulerColor = QColor(200, 200, 200);

    int ow = 1;
    if (config::base().getValue("graphics.point.outline_width", ow))
        _vstyle.pointOutlineWidth = ow;
    else
        _vstyle.pointOutlineWidth = 1;

    if (!config::base().getValue("graphics.label_font_pt", _vstyle.labelFontPt))
        _vstyle.labelFontPt = 0;

    QString font;
    if (config::base().getValue("graphics.label_font_family", font))
        _vstyle.labelFont = font;
    else
        _vstyle.labelFont.clear();


    applyStyle_AllDocuments();
}

void MainWindow::saveVisualStyle() const
{
    config::base().setValue("graphics.line_width", _vstyle.lineWidth);
    config::base().setValue("graphics.handle_size", _vstyle.handleSize);
    config::base().setValue("graphics.handle_pick_radius", _ghostPickRadius);
    config::base().setValue("graphics.edge_pick_radius", _edgePickRadius);
    config::base().setValue("graphics.number_font_pt", _vstyle.numberFontPt);
    config::base().setValue("graphics.show_numbers", _vstyle.showNumbers);
    config::base().setValue("graphics.show_selection_frame", _vstyle.showSelectionFrame);
    config::base().setValue("graphics.point_size", _vstyle.pointSize);
    config::base().setValue("graphics.handle_color", _vstyle.handleColor.name(QColor::HexArgb));
    config::base().setValue("graphics.selected_handle_color", _vstyle.selectedHandleColor.name(QColor::HexArgb));
    config::base().setValue("graphics.number_color", _vstyle.numberColor.name(QColor::HexArgb));
    config::base().setValue("graphics.number_bg_color", _vstyle.numberBgColor.name(QColor::HexArgb));
    config::base().setValue("graphics.colors.rectangle", _vstyle.rectangleLineColor.name(QColor::HexArgb));
    config::base().setValue("graphics.colors.circle", _vstyle.circleLineColor.name(QColor::HexArgb));
    config::base().setValue("graphics.colors.polyline", _vstyle.polylineLineColor.name(QColor::HexArgb));
    config::base().setValue("graphics.colors.line",  _vstyle.lineLineColor.name(QColor::HexArgb));
    config::base().setValue("graphics.colors.point", _vstyle.pointColor.name(QColor::HexArgb));
    config::base().setValue("graphics.colors.ruler", _vstyle.rulerColor.name(QColor::HexArgb));
    config::base().setValue("graphics.point.outline_width", _vstyle.pointOutlineWidth);
    config::base().setValue("graphics.fill_shape_when_selected", _vstyle.fillShapeWhenSelected);

    if (_vstyle.labelFontPt > 0)
        config::base().setValue("graphics.label_font_pt", _vstyle.labelFontPt);

    if (!_vstyle.labelFont.isEmpty())
        config::base().setValue("graphics.label_font_family", _vstyle.labelFont);
}

void MainWindow::applyStyle_AllDocuments()
{
    for (QMap<QString, Document::Ptr>::iterator it = _documentsMap.begin(); it != _documentsMap.end(); ++it)
    {
        Document::Ptr doc = it.value();
        if (!doc || !doc->scene)
            continue;

        doc->scene->setProperty("fillShapeWhenSelected", _vstyle.fillShapeWhenSelected);

        apply_LineWidth_ToScene(doc->scene);
        apply_PointSize_ToScene(doc->scene);
        apply_NumberSize_ToScene(doc->scene);
        updateLineColorsForScene(doc->scene);

        for (QGraphicsItem* item : doc->scene->items())
        {
            apply_PointStyle_ToItem(item);

            const QString cls = item ? item->data(0).toString() : QString();
            if (!cls.isEmpty())
                applyClassColorToItem(item, cls);
        }
        doc->scene->update();
    }
}

void MainWindow::forEachScene(std::function<void(QGraphicsScene*)> sceneHandler)
{
    for (QMap<QString, Document::Ptr>::iterator it = _documentsMap.begin(); it != _documentsMap.end(); ++it)
    {
        Document::Ptr doc = it.value();
        if (!doc)
            continue;

        QGraphicsScene* sc = doc->scene;
        if (!sc)
            continue;
        sceneHandler(sc);
    }
}

void MainWindow::apply_LineWidth_ToScene(QGraphicsScene* scene)
{
    if (!scene)
    {
        forEachScene([this](QGraphicsScene* scene){
            for (QGraphicsItem* item : scene->items())
                apply_LineWidth_ToItem(item);
            scene->update();
        });
        return;
    }
    for (QGraphicsItem* it : scene->items())
        apply_LineWidth_ToItem(it);
    scene->update();
}

void MainWindow::apply_PointSize_ToScene(QGraphicsScene* scene)
{
    if (!scene)
    {
        forEachScene([this](QGraphicsScene* scene){
            for (QGraphicsItem* item : scene->items())
                apply_PointSize_ToItem(item);
            scene->update();
        });
        return;
    }
    for (QGraphicsItem* it : scene->items())
        apply_PointSize_ToItem(it);
    scene->update();
}

void MainWindow::apply_NumberSize_ToScene(QGraphicsScene* scene)
{
    std::function<void(QGraphicsScene*)> applyForScene = [this](QGraphicsScene* scene)
    {
        if (!scene)
            return;

        for (QGraphicsItem* it : scene->items())
            apply_NumberSize_ToItem(it);
        scene->update();
    };
    if (!scene)
    {
        forEachScene([&](QGraphicsScene* scene){
            applyForScene(scene);
        });
        return;
    }

    applyForScene(scene);
}

void MainWindow::apply_LineWidth_ToItem(QGraphicsItem* item)
{
    if (!item)
        return;

    const qreal width = _vstyle.lineWidth;

    if (qgraph::Rectangle* rect = dynamic_cast<qgraph::Rectangle*>(item))
    {
        QPen pen = rect->pen();
        pen.setWidthF(width);
        pen.setCosmetic(true);

        const QString className = rect->data(0).toString();
        const QColor classColor = classColorFor(className);
        pen.setColor(classColor.isValid() ? classColor : _vstyle.rectangleLineColor);

        rect->setPen(pen);
        return;
    }
    if (qgraph::Circle* circle = dynamic_cast<qgraph::Circle*>(item))
    {
        QPen pen = circle->pen();
        pen.setWidthF(width);
        pen.setCosmetic(true);

        const QString className = circle->data(0).toString();
        const QColor classColor = classColorFor(className);
        pen.setColor(classColor.isValid() ? classColor : _vstyle.circleLineColor);

        circle->setPen(pen);
        circle->applyLineStyle(width);
        circle->setGlobalSelectionRectVisible(_vstyle.showSelectionFrame);
        return;
    }
    if (qgraph::Polyline* pline= dynamic_cast<qgraph::Polyline*>(item))
    {
        QPen pen = pline->pen();
        pen.setWidthF(width);
        pen.setCosmetic(true);

        const QString className = pline->data(0).toString();
        const QColor classColor = classColorFor(className);
        pen.setColor(classColor.isValid() ? classColor : _vstyle.polylineLineColor);

        pline->setPen(pen);
        pline->setGlobalSelectionRectVisible(_vstyle.showSelectionFrame);
        return;
    }
    if (qgraph::Line* line = dynamic_cast<qgraph::Line*>(item))
    {
        QPen pen = line->pen();
        pen.setWidthF(_vstyle.lineWidth);
        pen.setCosmetic(true);

        const QString className = line->data(0).toString();
        const QColor classColor = classColorFor(className);
        pen.setColor(classColor.isValid() ? classColor : _vstyle.lineLineColor);

        line->setPen(pen);
        line->setGlobalSelectionRectVisible(_vstyle.showSelectionFrame);
        return;
    }
    if (qgraph::Point* point = dynamic_cast<qgraph::Point*>(item))
    {
        QPen pen = point->pen();
        pen.setWidthF(_vstyle.pointOutlineWidth); // Отдельная толщина для точки
        pen.setCosmetic(true);

        const QString className = point->data(0).toString();
        const QColor classColor = classColorFor(className);
        pen.setColor(classColor.isValid() ? classColor : _vstyle.pointOutlineWidth);

        point->setPen(pen);
        return;
    }

    // Базовые Qt‑элементы
    if (QGraphicsLineItem* line = dynamic_cast<QGraphicsLineItem*>(item))
    {
        QPen pen = line->pen();
        pen.setWidthF(width);
        pen.setCosmetic(true);
        line->setPen(pen);
        return;
    }

    if (QGraphicsPathItem* pline = dynamic_cast<QGraphicsPathItem*>(item))
    {
        QPen pen = pline->pen();
        pen.setWidthF(width);
        pen.setCosmetic(true);
        pline->setPen(pen);
        return;
    }
    if (QGraphicsEllipseItem* circle = dynamic_cast<QGraphicsEllipseItem*>(item))
    {
        QPen pen = circle->pen();
        pen.setWidthF(width);
        pen.setCosmetic(true);
        circle->setPen(pen);
        return;
    }
    if (QGraphicsRectItem* rect = dynamic_cast<QGraphicsRectItem*>(item))
    {
        QPen pen = rect->pen();
        pen.setWidthF(width);
        pen.setCosmetic(true);
        rect->setPen(pen);
        return;
    }
}

void MainWindow::apply_PointSize_ToItem(QGraphicsItem* item)
{
    if (!item)
        return;

    // Ручки обычно лежат детьми нужной фигуры
    for (QGraphicsItem* ch : item->childItems())
    {
        if (qgraph::DragCircle* handle = dynamic_cast<qgraph::DragCircle*>(ch))
        {
            const bool isPoint = (dynamic_cast<qgraph::Point*>(item) != nullptr);

            // Размер узла для Point должен следовать размеру точки
            const qreal pointHandleSize = std::max<qreal>(_vstyle.pointSize, 4.0);

            handle->setBaseStyle(_vstyle.handleColor, isPoint ? pointHandleSize : _vstyle.handleSize);
            handle->setInteractionRadius(_ghostPickRadius);
            handle->setSelectedHandleColor(_vstyle.selectedHandleColor);
            handle->restoreBaseStyle();
            qgraph::DragCircle::rememberCurrentAsBase(handle);
        }
    }
}

void MainWindow::apply_NumberSize_ToItem(QGraphicsItem* item)
{
    if (!item)
        return;

    // Для классов фигур
    if (qgraph::Rectangle* rectangle = dynamic_cast<qgraph::Rectangle*>(item))
    {
        rectangle->setGlobalPointNumbersVisible(_vstyle.showNumbers);
        rectangle->applyNumberStyle(_vstyle.numberFontPt, _vstyle.numberColor, _vstyle.numberBgColor);
        return;
    }
    if (qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(item))
    {
        polyline->setGlobalPointNumbersVisible(_vstyle.showNumbers);
        polyline->applyNumberStyle(_vstyle.numberFontPt, _vstyle.numberColor, _vstyle.numberBgColor);
        return;
    }
    if (qgraph::Line* line = dynamic_cast<qgraph::Line*>(item))
    {
        line->setGlobalPointNumbersVisible(_vstyle.showNumbers);
        line->applyNumberStyle(_vstyle.numberFontPt, _vstyle.numberColor, _vstyle.numberBgColor);
        return;
    }

    // Для стандартных Qt элементов
    QVector<QGraphicsSimpleTextItem*> texts;
    QVector<QGraphicsRectItem*> backgrounds;

    for (QGraphicsItem* ch : item->childItems())
    {
        if (QGraphicsSimpleTextItem* t = dynamic_cast<QGraphicsSimpleTextItem*>(ch))
        {
            // Проверяем, что это номер (короткое число)
            QString text = t->text();
            if (text.length() <= 3)
            {
                bool ok;
                text.toInt(&ok);
                if (ok) texts << t;
            }
        }
        else if (QGraphicsRectItem* bg = dynamic_cast<QGraphicsRectItem*>(ch))
        {
            if (bg->zValue() > 900 && bg->zValue() < 1010)
                backgrounds << bg;
        }
    }

    // Применяем стиль к текстам
    for (QGraphicsSimpleTextItem* t : texts)
    {
        if (!t) continue;

        QFont f = t->font();
        f.setPointSizeF(_vstyle.numberFontPt);
        t->setFont(f);
        t->setBrush(Qt::white);
        t->setPen(Qt::NoPen);
        t->setZValue(1001);
        t->setFlag(QGraphicsItem::ItemIgnoresParentOpacity, true);
    }

    // Обновляем фоны
    for (QGraphicsRectItem* bg : backgrounds)
    {
        if (!bg) continue;

        // Ищем ближайший текст
        QGraphicsSimpleTextItem* nearest = nullptr;
        qreal bestDistance = std::numeric_limits<qreal>::max();

        for (QGraphicsSimpleTextItem* t : texts)
        {
            if (!t) continue;

            qreal distance = QLineF(bg->pos(), t->pos()).length();
            if (distance < bestDistance)
            {
                bestDistance = distance;
                nearest = t;
            }
        }

        if (nearest)
        {
            bg->setRect(nearest->boundingRect());
            bg->setBrush(QBrush(QColor(0, 0, 0, 180)));
            bg->setPen(Qt::NoPen);
            bg->setPos(nearest->pos());
            bg->setZValue(1000);
            bg->setFlag(QGraphicsItem::ItemIgnoresParentOpacity, true);
        }
    }
}

void MainWindow::apply_PointStyle_ToItem(QGraphicsItem* item)
{
    if (!item)
        return;

    if (qgraph::Point* p = dynamic_cast<qgraph::Point*>(item))
    {

        const QString cls = p->data(0).toString();
        const QColor cc = classColorFor(cls);
        const QColor color = cc.isValid() ? cc : _vstyle.pointColor;

        const qreal diam = std::max(2, _vstyle.pointSize);
        p->setDotStyle(color, diam, _vstyle.handleSize);
    }
}

void MainWindow::applyLabelFontToUi()
{
    if (!ui || !ui->splitter)
        return;

    // Берем текущий шрифт правой зоны как базовый
    QFont f = ui->splitter->font();

    if (!_vstyle.labelFont.isEmpty())
        f.setFamily(_vstyle.labelFont);

    if (_vstyle.labelFontPt > 0)
        f.setPointSize(_vstyle.labelFontPt);

    // Применяем ко всей правой панели
    ui->splitter->setFont(f);

    if (ui->toolBar)
        ui->toolBar->setFont(f);
    if (ui->toolBar_2)
        ui->toolBar_2->setFont(f);

    ui->splitter->updateGeometry();
    ui->splitter->update();
}

void MainWindow::updateLineColorsForScene(QGraphicsScene* scene)
{
    if (!scene) return;

    for (QGraphicsItem* item : scene->items())
    {
        const QString cls = item->data(0).toString();
        const QColor classC = classColorFor(cls);
        const bool hasClassColor = classC.isValid();

        if (qgraph::Rectangle* rect = dynamic_cast<qgraph::Rectangle*>(item))
        {
            QPen pen = rect->pen();
            pen.setColor(hasClassColor ? classC : _vstyle.rectangleLineColor);
            rect->setPen(pen);
        }
        else if (qgraph::Circle* circle = dynamic_cast<qgraph::Circle*>(item))
        {
            QPen pen = circle->pen();
            pen.setColor(hasClassColor ? classC : _vstyle.circleLineColor);
            circle->setPen(pen);
        }
        else if (qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(item))
        {
            QPen pen = polyline->pen();
            pen.setColor(hasClassColor ? classC : _vstyle.polylineLineColor);
            polyline->setPen(pen);
        }
        else if (qgraph::Line* line = dynamic_cast<qgraph::Line*>(item))
        {
            QPen pen = line->pen();
            pen.setColor(hasClassColor ? classC : _vstyle.lineLineColor);
            line->setPen(pen);
        }
        else if (qgraph::Point* point = dynamic_cast<qgraph::Point*>(item))
        {
            QPen pen = point->pen();
            pen.setColor(hasClassColor ? classC : _vstyle.pointColor);
            pen.setWidthF(_vstyle.pointOutlineWidth);
            pen.setCosmetic(true);
            point->setPen(pen);
        }
    }
    scene->update();
}

void MainWindow::applyZoom(qreal zoomFactor)
{
    if (!ui->graphView) return;

    // Ограничим диапазон
    if (zoomFactor < _kMinZoom)
        zoomFactor = _kMinZoom;
    if (zoomFactor > _kMaxZoom)
        zoomFactor = _kMaxZoom;

    // Сохраним текущий центр сцены, чтобы картинка не «уезжала»
    const QPointF centerScene =
        ui->graphView->mapToScene(ui->graphView->viewport()->rect().center());

    QTransform t;
    t.scale(zoomFactor, zoomFactor);
    ui->graphView->setTransform(t);

    // Вернем центр
    ui->graphView->centerOn(centerScene);

    _m_zoom = zoomFactor;

    if (Document::Ptr doc = currentDocument())
    {
        doc->viewState = {
            ui->graphView->horizontalScrollBar()->value(),
            ui->graphView->verticalScrollBar()->value(),
            _m_zoom,
            centerScene
        };
    }
}

void MainWindow::applyClosePolyline()
{
    using SDM = Settings::PolylineCloseMode;
    using PLM = qgraph::Polyline::CloseMode;

    qgraph::Polyline::CloseMode mode = PLM::DoubleClick;
    switch (_polylineCloseMode)
    {
        case SDM::DoubleClick:             mode = PLM::DoubleClick;   break;
        case SDM::SingleClickOnFirstPoint: mode = PLM::SingleClickOnFirstPoint; break;
        case SDM::CtrlModifier:            mode = PLM::CtrlModifier;            break;
        case SDM::KeyCWithoutNewPoint:     mode = PLM::KeyC;                    break;
    }
    qgraph::Polyline::setGlobalCloseMode(mode);
    if (ui && ui->actClosePolyline)
        ui->actClosePolyline->setShortcut(mode == PLM::KeyC ? QKeySequence(Qt::Key_C) : QKeySequence());
}

void MainWindow::applyFinishLine()
{
    using SDM = Settings::LineFinishMode;
    using PLM = qgraph::Line::CloseMode;

    qgraph::Line::CloseMode mode = PLM::DoubleClick;
    switch (_lineFinishMode)
    {
        case SDM::DoubleClick: mode = PLM::DoubleClick;
            break;
        case SDM::CtrlModifier: mode = PLM::CtrlModifier;
            break;
        case SDM::KeyCWithoutNewPoint: mode = PLM::KeyC;
            break;
    }
    qgraph::Line::setGlobalCloseMode(mode);
}

QUndoStack* MainWindow::activeUndoStack() const
{
    if (!_undoGroup)
        return nullptr;

    if (QUndoStack* stack = _undoGroup->activeStack())
        return stack;

    // Запасной вариант - стек текущего документа
    if (Document::Ptr doc = currentDocument())
        return doc->_undoStack ? doc->_undoStack.get() : nullptr;

    return nullptr;
}

void MainWindow::restoreDrawingStateAfterStackChange()
{
    Document::Ptr doc = currentDocument();

    if (!doc || !doc->scene)
        return;

    qgraph::Line* openLine = nullptr;
    qgraph::Polyline* openPolyline = nullptr;

    if (_polyline && _polyline->scene() == doc->scene && !_polyline->isClosed())
        openPolyline = _polyline;

    if (_line && _line->scene() == doc->scene && !_line->isClosed())
        openLine = _line;

    if (!openPolyline && !openLine)
    {
        const QList<QGraphicsItem*> sceneItems = doc->scene->items();

        for (QGraphicsItem* item : sceneItems)
        {
            if (!item)
                continue;

            if (doc->polygonList.items.contains(item))
                continue;

            qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(item);

            if (polyline && !polyline->isClosed())
            {
                openPolyline = polyline;
                break;
            }

            qgraph::Line* line = dynamic_cast<qgraph::Line*>(item);

            if (line && !line->isClosed())
                openLine = line;
        }
    }

    if (openPolyline)
    {
        _line = nullptr;
        _polyline = openPolyline;

        _drawingLine = false;
        _drawingPolyline = true;
        _isInDrawingMode = true;

        _resumeEditing = false;
        _resumeUid = 0;
        _pendingDrawTool = PendingDrawTool::None;

        _polyline->setFlag(QGraphicsItem::ItemIsMovable, false);
        _polyline->setSelected(true);
        _polyline->setFocus();
        _polyline->updatePointNumbers();

        apply_LineWidth_ToItem(_polyline);
        apply_PointSize_ToItem(_polyline);
        apply_NumberSize_ToItem(_polyline);

        setSceneItemsMovable(false);
        updateModeLabel();

        return;
    }

    if (openLine)
    {
        _polyline = nullptr;
        _line = openLine;

        _drawingPolyline = false;
        _drawingLine = true;
        _isInDrawingMode = true;

        _resumeEditing = false;
        _resumeUid = 0;
        _pendingDrawTool = PendingDrawTool::None;

        _line->setFlag(QGraphicsItem::ItemIsMovable, false);
        _line->setSelected(true);
        _line->setFocus();
        _line->updatePointNumbers();

        apply_LineWidth_ToItem(_line);
        apply_PointSize_ToItem(_line);
        apply_NumberSize_ToItem(_line);

        setSceneItemsMovable(false);
        updateModeLabel();

        return;
    }

    if (_drawingLine || _drawingPolyline)
    {
        _line = nullptr;
        _polyline = nullptr;

        _drawingLine = false;
        _drawingPolyline = false;
        _isInDrawingMode = false;

        _resumeEditing = false;
        _resumeUid = 0;
        _pendingDrawTool = PendingDrawTool::None;

        setSceneItemsMovable(true);
        updateModeLabel();
    }
}

void MainWindow::setPolygonListModelForCurrentDocument()
{
    Document::Ptr doc = currentDocument();
    if (!doc) return;

    ui->polygonList->setModel(&doc->polygonList.model);

    if (QItemSelectionModel* selectionModel = ui->polygonList->selectionModel())
    {
        chk_connect_a(selectionModel, &QItemSelectionModel::selectionChanged,
                      this, &MainWindow::onPolygonListSelectionChanged)

        connect(selectionModel, &QItemSelectionModel::currentChanged,
                this, &MainWindow::updateShapeListButtons,
                Qt::UniqueConnection);
    }

    updateShapeListButtons();
}

void MainWindow::removeSceneAndListItems(const QVector<QGraphicsItem*>& items)
{
    for (QGraphicsItem* item : items)
    {
        if (!item)
            continue;

        removeListEntryBySceneItem(item);

        if (item->scene())
            item->scene()->removeItem(item);

        clearLinePolylineStateForDeletedItem(item);
        delete item;
    }
}

void MainWindow::removeListEntryBySceneItem(QGraphicsItem* sceneItem)
{
    if (!sceneItem)
        return;

    Document::Ptr doc = currentDocument();
    if (!doc) return;

    const int row = polygonListRowByItem(sceneItem);

    if (row < 0)
        return;

    doc->polygonList.items.removeAt(row);
    doc->polygonList.model.removeRow(row);

    renumberPolygonList();
}

QGraphicsItem* MainWindow::sceneItemFromListIndex(const QModelIndex& index) const
{
    if (!index.isValid())
        return nullptr;

    return sceneItemFromListRow(index.row());
}

QGraphicsItem* MainWindow::sceneItemFromListRow(int row) const
{
    Document::Ptr doc = currentDocument();

    if (!doc)
        return nullptr;

    if (row < 0 || row >= doc->polygonList.items.size())
        return nullptr;

    return doc->polygonList.items[row];
}

int MainWindow::polygonListRowByItem(QGraphicsItem* sceneItem) const
{
    if (!sceneItem)
        return -1;

    Document::Ptr doc = currentDocument();

    if (!doc)
        return -1;

    for (int row = 0; row < doc->polygonList.items.size(); ++row)
    {
        if (doc->polygonList.items[row] == sceneItem)
            return row;
    }

    return -1;
}

QGraphicsItem* MainWindow::findItemByUid(qulonglong uid) const
{
    if (uid == 0)
        return nullptr;

    Document::Ptr doc = currentDocument();

    if (!doc || !doc->scene)
        return nullptr;

    const QList<QGraphicsItem*> itemsList = doc->scene->items();

    for (QGraphicsItem* item : itemsList)
    {
        if (!item)
            continue;

        if (item->data(_roleUid).toULongLong() == uid)
            return item;
    }

    return nullptr;
}

qulonglong MainWindow::ensureUid(QGraphicsItem* item) const
{
    if (!item)
        return 0;
    // Пытаемся получить уже назначенный uid из данных QGraphicsItem
    QVariant v = item->data(_roleUid);
    if (!v.isNull())
        return v.toULongLong();
    // Если uid еще не был назначен, берем следующий свободный номер
    qulonglong uid = _uidCounter++;
    item->setData(_roleUid, QVariant::fromValue<qulonglong>(uid));
    return uid;
}

void MainWindow::clearLinePolylineStateForDeletedItem(QGraphicsItem* item)
{
    if (!item)
        return;

    const qulonglong uid = item->data(_roleUid).toULongLong();

    const bool isLine = (dynamic_cast<qgraph::Line*>(item) != nullptr);
    const bool isPolyline = (dynamic_cast<qgraph::Polyline*>(item) != nullptr);

    if (isLine)
    {
        if (_line == item)
            _line = nullptr;
        _drawingLine = false;
    }

    if (isPolyline)
    {
        if (_polyline == item)
            _polyline = nullptr;
        _drawingPolyline = false;
    }

    if (_resumeEditing && (_resumeUid != 0) && (uid != 0) && (uid == _resumeUid))
    {
        _resumeEditing = false;
        _resumeUid = 0;
    }

    // Возврат в режим просмотра, если мы больше не рисуем
    if (!_drawingLine && !_drawingPolyline && (!_resumeEditing || _resumeUid == 0))
    {
        _isInDrawingMode = false;
        setSceneItemsMovable(true);
        updateModeLabel();
    }
}

void MainWindow::cancelRulerMode()
{
    _drawingRuler = false;
    _isDrawingRuler = false;
    _isInDrawingMode = false;

    clearAllHandleHoverEffects();
    hideGhost();
    setSceneItemsMovable(true);

    if (_rulerLine)
    {
        QGraphicsScene* scene = _rulerLine->scene();

        if (scene)
            scene->removeItem(_rulerLine);

        delete _rulerLine;
        _rulerLine = nullptr;
    }

    if (_rulerText)
    {
        QGraphicsScene* scene = _rulerText->scene();

        if (scene)
            scene->removeItem(_rulerText);

        delete _rulerText;
        _rulerText = nullptr;
    }

    updateModeLabel();
}

void MainWindow::toggleMenuBarVisible()
{
    const bool newVisible = !ui->menuBar->isVisible();
    ui->menuBar->setVisible(newVisible);
    bool keepMenuVis = false;
    config::base().getValue("ui.keep_menu_visibility", keepMenuVis);
    if (keepMenuVis)
    {
        config::base().setValue("ui.menu_visible", newVisible);
        config::base().saveFile();
    }
}

QColor MainWindow::classColorFor(const QString& className) const
{
    if (className.isEmpty())
        return QColor();

    const QMap<QString, QColor>::const_iterator it = _projectClassColors.find(className);
    if (it == _projectClassColors.end())
        return QColor();

    if (!it.value().isValid())
        return QColor();

    return it.value();
}

void MainWindow::applyClassColorToItem(QGraphicsItem* item, const QString& className)
{
    if (!item)
        return;

    const QColor classColor = classColorFor(className);

    QColor lineColor;
    if (dynamic_cast<qgraph::Rectangle*>(item))
        lineColor = classColor.isValid() ? classColor : _vstyle.rectangleLineColor;
    else if (dynamic_cast<qgraph::Circle*>(item))
        lineColor = classColor.isValid() ? classColor : _vstyle.circleLineColor;
    else if (dynamic_cast<qgraph::Polyline*>(item))
        lineColor = classColor.isValid() ? classColor : _vstyle.polylineLineColor;
    else if (dynamic_cast<qgraph::Line*>(item))
        lineColor = classColor.isValid() ? classColor : _vstyle.lineLineColor;
    else if (dynamic_cast<qgraph::Point*>(item))
        lineColor = classColor.isValid() ? classColor : _vstyle.pointColor;
    else
        return;

    QColor fillColor = lineColor;
    fillColor.setAlpha(60);

    const QBrush fillBrush =
        _vstyle.fillShapeWhenSelected ? QBrush(fillColor)
                                   : QBrush(Qt::transparent);

    const QBrush noBrush {Qt::transparent};

    if (qgraph::Rectangle* rect = dynamic_cast<qgraph::Rectangle*>(item))
    {
        QPen pen = rect->pen();
        pen.setColor(lineColor);
        pen.setWidthF(_vstyle.lineWidth);
        pen.setCosmetic(true);
        rect->setPen(pen);

        rect->setBrush(rect->isSelected() ? fillBrush : noBrush);
        return;
    }

    if (qgraph::Circle* circle = dynamic_cast<qgraph::Circle*>(item))
    {
        QPen pen = circle->pen();
        pen.setColor(lineColor);
        pen.setWidthF(_vstyle.lineWidth);
        pen.setCosmetic(true);
        circle->setPen(pen);

        circle->setBrush(circle->isSelected() ? fillBrush : noBrush);

        for (QGraphicsItem* ch : circle->childItems())
        {
            if (QGraphicsLineItem* lineCenter = dynamic_cast<QGraphicsLineItem*>(ch))
            {
                QPen pen = lineCenter->pen();
                pen.setColor(lineColor);
                pen.setWidthF(_vstyle.lineWidth);
                pen.setCosmetic(true);
                lineCenter->setPen(pen);
            }
        }
        return;
    }

    if (qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(item))
    {
        QPen pen = polyline->pen();
        pen.setColor(lineColor);
        pen.setWidthF(_vstyle.lineWidth);
        pen.setCosmetic(true);
        polyline->setPen(pen);

        polyline->setBrush(polyline->isSelected() ? fillBrush : noBrush);
        return;
    }

    if (qgraph::Line* line = dynamic_cast<qgraph::Line*>(item))
    {
        QPen pen = line->pen();
        pen.setColor(lineColor);
        pen.setWidthF(_vstyle.lineWidth);
        pen.setCosmetic(true);
        line->setPen(pen);

        line->setBrush(line->isSelected() ? fillBrush : noBrush);
        return;
    }

    if (qgraph::Point* point = dynamic_cast<qgraph::Point*>(item))
    {
        const qreal diam = std::max(2, _vstyle.pointSize);
        point->setDotStyle(lineColor, diam, _vstyle.handleSize);
        return;
    }
}

void MainWindow::updateImageSizeLabel(const QSize& size)
{
    if (!_imageSizeLabel)
        return;

    if (!size.isValid())
    {
        _imageSizeLabel->setText(u8"Размер изображения: —");
        return;
    }

    _imageSizeLabel->setText(QString(u8"Размер изображения: %1 × %2")
                            .arg(size.width())
                            .arg(size.height()));
}

void MainWindow::startMergeLinesMode()
{
    _mergeLinesMode = true;
    _mergeLineA = nullptr;
    _mergeLineAEndIdx = -1;

    if (statusBar())
        statusBar()->showMessage(u8"Объединение линий: кликните по КОНЕЧНОЙ точке первой линии", 0);

    updateModeLabel();
}

void MainWindow::cancelMergeLinesMode()
{
    _mergeLinesMode = false;
    _mergeLineA = nullptr;
    _mergeLineAEndIdx = -1;

    if (statusBar())
        statusBar()->showMessage(u8"Объединение линий отменено", 2000);

    updateModeLabel();
}

bool MainWindow::handleMergeLinesClick(const QPointF& scenePos)
{
    if (!_mergeLinesMode)
        return false;

    qgraph::DragCircle* h = pickHandleAt(scenePos);
    if (!h || !h->isValid())
        return false;

    QGraphicsItem* parent = h->parentItem();
    qgraph::Line* line = dynamic_cast<qgraph::Line*>(parent);
    if (!line)
        return false;

    const QVector<qgraph::DragCircle*>& circles = line->circles();
    const int idx = circles.indexOf(h);
    if ((idx != 0) && (idx != circles.size() - 1))
        return false; // Нужен только конец линии

    if (!_mergeLineA)
    {
        _mergeLineA = line;
        _mergeLineAEndIdx = idx;

        if (statusBar())
            statusBar()->showMessage(u8"Объединение линий: теперь кликните по КОНЕЧНОЙ точке второй линии", 0);

        return true;
    }

    // Второй клик
    if (line == _mergeLineA)
        return true; // Игнорируем “в ту же линию”, но клик съедаем чтобы не двигать/редактировать

    const bool ok = performMergeLines(_mergeLineA, _mergeLineAEndIdx, line, idx);
    cancelMergeLinesMode();
    return ok ? true : true;
}

bool MainWindow::   performMergeLines(qgraph::Line* lineA, int indexA, qgraph::Line* lineB, int indexB)
{
    Document::Ptr doc = currentDocument();
    if (!doc || !doc->_undoStack || !lineA || !lineB)
        return false;

    if (lineA == lineB)
        return false;

    // Только концы
    const int aLast = lineA->circles().size() - 1;
    const int bLast = lineB->circles().size() - 1;

    if (!((indexA == 0) || (indexA == aLast)))
        return false;

    if (!((indexB == 0) || (indexB == bLast)))
        return false;

    // Класс должен совпадать
    const QString labelA = lineA->data(0).toString();
    const QString labelB = lineB->data(0).toString();
    if (!labelA.isEmpty()
        && !labelB.isEmpty()
        && labelA != labelB)
    {
        messageBox(
            this,
            QMessageBox::Warning,
            QString(u8"Нельзя объединить линии разных классов:\n\"%1\" и \"%2\"").arg(labelA, labelB)
        );
        return false;
    }
    const QString outLabel = !labelA.isEmpty() ? labelA : labelB;

    // Собираем точки
    QVector<QPointF> pointsA = lineA->points();
    QVector<QPointF> pointsB = lineB->points();

    if (pointsA.size() < 2 || pointsB.size() < 2)
        return false;

    // Разворачиваем так, чтобы выбранный конец A стал последним,
    // а выбранный конец B стал первым.
    if (indexA == 0)
        std::reverse(pointsA.begin(), pointsA.end());
    if (indexB == bLast)
        std::reverse(pointsB.begin(), pointsB.end());

    // Склейка + удаление дубликата в месте стыка
    QVector<QPointF> merged;
    merged.reserve(pointsA.size() + pointsB.size());
    merged += pointsA;

    const QPointF lastA = merged.last();
    const QPointF firstB = pointsB.first();

    const qreal eps = 2.0;
    if (QLineF(lastA, firstB).length() < eps)
    {
        // Не добавляем первый узел B
        for (int i = 1; i < pointsB.size(); ++i)
            merged.push_back(pointsB[i]);
    }
    else
    {
        merged += pointsB;
    }

    if (merged.size() < 2)
        return false;

    void (*rotateToIndex)(QVector<QPointF>&, int) =
        [](QVector<QPointF>& points, int k)
    {
        if (points.isEmpty())
            return;

        k %= points.size();
        if (k < 0)
            k += points.size();
        if (k == 0)
            return;

        QVector<QPointF> tmp;
        tmp.reserve(points.size());
        for (int i = 0; i < points.size(); ++i)
            tmp.push_back(points[(k + i) % points.size()]);
        points = std::move(tmp);
    };

    // Нормализация нумерации
    const qreal epsClose = 6.0;
    const bool looksClosed =
        (merged.size() >= 3) && (QLineF(merged.first(), merged.last()).length() < epsClose);

    const double a2 = signedArea2(merged);

    const bool numberingFromLast = (a2 > 0.0);

    if (looksClosed)
    {
        // Старт - верхний-левый
        const int k = topLeftIndex(merged);
        rotateToIndex(merged, k);

        // Если после поворота первый и последний почти совпали, то убираем дубль
        if (merged.size() >= 2 && QLineF(merged.first(), merged.last()).length() < epsClose)
            merged.pop_back();
    }
    else
    {
        // Открытая линия: старт - верхне-левый конец
        if (isBetterTopLeft(merged.last(), merged.first()))
            std::reverse(merged.begin(), merged.end());
    }

    undo::LineTopology::Data data;
    data.type = undo::LineTopology::Type::Merge;
    data.resultPoints.append(merged);
    data.className = outLabel;
    data.visible = lineA->isVisible() && lineB->isVisible();
    data.zValue = qMax(lineA->zValue(), lineB->zValue());
    data.numberingFromLast = numberingFromLast;

    QVector<qgraph::Line*> sourceLines;
    sourceLines.append(lineA);
    sourceLines.append(lineB);

    undo::LineTopology* command = new undo::LineTopology(doc.get(),
                                                         sourceLines,
                                                         data,
                                                         u8"Объединить линии");

    if (!command->isValid())
    {
        delete command;
        return false;
    }

    QUndoStack* stack = activeUndoStack();

    if (!stack)
    {
        delete command;
        return false;
    }

    stack->push(command);

    doc->isModified = true;
    updateFileListDisplay(doc->filePath);

    return true;
}

bool MainWindow::performSplitLineByEdge(qgraph::Line* line, const QPointF& scenePos)
{
    Document::Ptr doc = currentDocument();
    QUndoStack* stack = activeUndoStack();

    if (!doc || !stack || !line)
        return false;

    const QVector<QPointF> points = line->points();
    const int n = points.size();

    if (n < 4)
    {
        messageBox(
            this,
            QMessageBox::Warning,
            u8"Нельзя разорвать линию: нужно минимум 4 точки"
        );
        return false;
    }

    double (*dist2PointToSegment)(const QPointF&, const QPointF&, const QPointF&) =
        [](const QPointF& p, const QPointF& a, const QPointF& b) -> double
    {
        const double vx = b.x() - a.x();
        const double vy = b.y() - a.y();
        const double wx = p.x() - a.x();
        const double wy = p.y() - a.y();
        const double vv = vx*vx + vy*vy;
        double t = 0.0;
        if (vv > 1e-12)
            t = (wx*vx + wy*vy) / vv;
        if (t < 0.0) t = 0.0;
        if (t > 1.0) t = 1.0;
        const double px = a.x() + t*vx;
        const double py = a.y() + t*vy;
        const double dx = p.x() - px;
        const double dy = p.y() - py;
        return dx*dx + dy*dy;
    };

    // Ищем ребро (i, i+1), ближайшее к клику
    int bestIdx = 0;
    double bestD2 = std::numeric_limits<double>::infinity();
    for (int i = 0; i < n - 1; ++i)
    {
        // const QPointF& firstPt = before.points[i];
        // const QPointF& secondPt = before.points[i + 1];

        const QPointF& firstPt = points[i];
        const QPointF& secondPt = points[i + 1];

        const double d2 = dist2PointToSegment(scenePos, firstPt, secondPt);
        if (d2 < bestD2)
        {
            bestD2 = d2;
            bestIdx = i;
        }
    }

    // Чтобы обе линии были валидны (>=2 точки), ребро не должно быть крайним
    if (bestIdx < 1 || bestIdx > n - 3)
    {
        messageBox(
            this,
            QMessageBox::Warning,
            u8"Нельзя разорвать по этому ребру: получится линия из одной точки.  "
            u8"Выберите внутреннее ребро"
        );
        return false;
    }

    QVector<QPointF> pts1;
    pts1.reserve(bestIdx + 1);

    for (int i = 0; i <= bestIdx; ++i)
        pts1.push_back(points[i]);

    QVector<QPointF> pts2;
    pts2.reserve(n - (bestIdx + 1));

    for (int i = bestIdx + 1; i < n; ++i)
        pts2.push_back(points[i]);

    if (pts1.size() < 2 || pts2.size() < 2)
        return false;

    undo::LineTopology::Data data;
    data.type = undo::LineTopology::Type::Split;
    data.resultPoints.append(pts1);
    data.resultPoints.append(pts2);
    data.className = line->data(0).toString();
    data.visible = line->isVisible();
    data.zValue = line->zValue();
    data.numberingFromLast = line->isNumberingFromLast();

    QVector<qgraph::Line*> sourceLines;
    sourceLines.append(line);

    undo::LineTopology* command = new undo::LineTopology(doc.get(),
                                                         sourceLines,
                                                         data,
                                                         u8"Разрыв линии");

    if (!command->isValid())
    {
        delete command;
        return false;
    }

    stack->push(command);

    doc->isModified = true;
    updateFileListDisplay(doc->filePath);

    return true;
}

// Формула шнуровки
double MainWindow::signedArea2(const QVector<QPointF>& points)
{
    // Удвоенная ориентированная площадь для замкнутого контура
    // Без дублирования первой точки в конце
    const int n = points.size();
    if (n < 3)
        return 0.0;

    double s = 0.0;
    for (int i = 0; i < n; ++i)
    {
        const QPointF& a = points[i];
        const QPointF& b = points[(i + 1) % n];
        s += (double)a.x() * (double)b.y() - (double)b.x() * (double)a.y();
    }
    return s;
}

int MainWindow::topLeftIndex(const QVector<QPointF>& points)
{
    if (points.isEmpty()) return -1;
    int best = 0;
    for (int i = 1; i < points.size(); ++i)
        if (isBetterTopLeft(points[i], points[best]))
            best = i;
    return best;
}

bool MainWindow::isBetterTopLeft(const QPointF& firstPoint, const QPointF& secondPoint)
{
    // “верхний-левый”: меньше y, при равных y - меньше x
    if (firstPoint.y() < secondPoint.y())
        return true;

    if (firstPoint.y() > secondPoint.y())
        return false;

    return firstPoint.x() < secondPoint.x();
}

void MainWindow::updateModeLabel()
{
    if (!_modeLabel) return;

    if (_drawingRuler || _isDrawingRuler)
    {
        _modeLabel->setText(_isDrawingRuler
                            ? u8"Режим: линейка (измерение...)"
                            : u8"Режим: линейка");
        return;
    }
    if (_isInDrawingMode
        || _isDrawingRectangle
        || _isDrawingCircle
        || _isDrawingLine
        || _isDrawingPolyline
        || _isDrawingPoint
        || _drawingRectangle
        || _drawingCircle
        || _drawingLine
        || _drawingPolyline
        || _drawingPoint)
    {
        QString tool = u8"фигура";
        if (_isDrawingRectangle || _drawingRectangle)
            tool = u8"прямоугольник";
        else if (_isDrawingCircle || _drawingCircle)
            tool = u8"круг";
        else if (_isDrawingPolyline || _drawingPolyline)
            tool = u8"полилиния";
        else if (_isDrawingLine || _drawingLine)
            tool = u8"линия";
        else if (_isDrawingPoint || _drawingPoint)
            tool = u8"точка";

        _modeLabel->setText(QString(u8"Режим: рисование (%1)").arg(tool));
        return;
    }
    if (_zoomRectActive)
    {
        _modeLabel->setText(u8"Режим: зум-область");
        return;
    }
    if (_mergeLinesMode)
    {
        _modeLabel->setText(u8"Режим: соединение линий");
        return;
    }
    // if (_shiftImageDragging)
    // {
    //     _modeLabel->setText(u8"Режим: сдвиг изображения"));
    //     return;
    // }
    if (_isDraggingImage)
    {
        _modeLabel->setText(_isAllMoved
                            ? u8"Режим: перемещение (изображение + разметка)"
                            : u8"Режим: перемещение (изображение)");
        return;
    }
    if (_editInProgress || _handleDragging || _m_isDraggingHandle || _ghostActive)
    {
        _modeLabel->setText(u8"Режим: редактирование");
        return;
    }
    if (_resumeEditing)
    {
        _modeLabel->setText(u8"Режим: продолжение");
        return;
    }
    _modeLabel->setText(u8"Режим: просмотр");
}

void MainWindow::handlePolylineLmbClick(const QPointF& scenePos,
                                        Qt::KeyboardModifiers mods,
                                        GraphicsView* graphView)
{
    Q_UNUSED(graphView);

    const Qt::KeyboardModifiers closeMods = mods | QApplication::keyboardModifiers();

    _isInDrawingMode = true;
    setSceneItemsMovable(false);
    updateModeLabel();
    _startPoint = scenePos;

    Document::Ptr doc = currentDocument();
    QGraphicsScene* scene = doc->scene;

    if (!_polyline)
    {
        _resumeEditing = false;
        _resumeUid = 0;
        // Если полилиния еще не создана, создаем объект
        _polyline = new qgraph::Polyline(scene, _startPoint);
        qgraph::Polyline* createdPolyline = _polyline;

        ensureUid(createdPolyline);
        apply_LineWidth_ToItem(createdPolyline);
        apply_PointSize_ToItem(createdPolyline);
        apply_NumberSize_ToItem(createdPolyline);

        createdPolyline->setFlag(QGraphicsItem::ItemIsMovable, false);
        createdPolyline->setSelected(true);
        createdPolyline->setFocus();
        createdPolyline->updatePointNumbers();

        createdPolyline->setModificationCallback([this]()
        {
            if (_drawingPolyline && _polyline && _polyline->isClosed())
            {
                _drawingPolyline = false;
                _isDrawingPolyline = false;
                _isInDrawingMode = false;

                setSceneItemsMovable(true);
                updateModeLabel();

                _polyline->updatePointNumbers();
                apply_LineWidth_ToItem(_polyline);
                apply_PointSize_ToItem(_polyline);
                apply_NumberSize_ToItem(_polyline);

                if (!_resumeEditing)
                {
                    SelectClass dialog(_projectClasses, this);

                    if (dialog.exec() == QDialog::Accepted)
                    {
                        const QString selectedClass = dialog.selectedClass();

                        if (!selectedClass.isEmpty())
                        {
                            _polyline->setClosed(true, false);
                            _polyline->setFlag(QGraphicsItem::ItemIsMovable, true);

                            _polyline->setData(0, selectedClass);
                            applyClassColorToItem(_polyline, selectedClass);
                            linkSceneItemToList(_polyline);

                            Document::Ptr doc = currentDocument();

                            if (doc)
                            {
                                if (QUndoStack* stack = activeUndoStack())
                                {
                                    stack->push(new undo::FinishDrawing(doc.get(),
                                                                        _polyline,
                                                                        undo::FinishDrawing::Type::Polyline,
                                                                        u8"Добавление полилинии"));
                                }

                                if (!doc->isModified)
                                {
                                    doc->isModified = true;
                                    updateFileListDisplay(doc->filePath);
                                }
                            }

                            _drawingPolyline = false;
                            _resumeEditing = false;
                            _resumeUid = 0;
                            _polyline = nullptr;

                            _isInDrawingMode = false;
                            setSceneItemsMovable(true);

                            updateModeLabel();
                            raiseAllHandlesToTop();
                        }
                    }
                }
                else
                {
                    _polyline->setClosed(true, false);
                    _polyline->setFlag(QGraphicsItem::ItemIsMovable, true);

                    const QString className = _polyline->data(0).toString();

                    if (!className.isEmpty())
                        applyClassColorToItem(_polyline, className);

                    _polyline->updatePointNumbers();
                    apply_LineWidth_ToItem(_polyline);
                    apply_PointSize_ToItem(_polyline);
                    apply_NumberSize_ToItem(_polyline);

                    Document::Ptr doc = currentDocument();

                    if (doc)
                    {
                        if (QUndoStack* stack = activeUndoStack())
                        {
                            undo::ResumeEdit::Data data;
                            data.type = undo::ResumeEdit::Type::Polyline;
                            data.resumeIndex = -1;
                            data.removedPoints.clear();
                            data.addedPoints.clear();

                            data.closedBefore = false;
                            data.closedAfter = true;

                            stack->push(new undo::ResumeEdit(doc.get(),
                                                             _polyline,
                                                             data,
                                                             u8"Завершение продолжения полилинии"));
                        }

                        if (!doc->isModified)
                        {
                            doc->isModified = true;
                            updateFileListDisplay(doc->filePath);
                        }
                    }

                    _drawingPolyline = false;
                    _resumeEditing = false;
                    _resumeUid = 0;
                    _polyline = nullptr;

                    _isInDrawingMode = false;
                    setSceneItemsMovable(true);

                    updateModeLabel();
                    raiseAllHandlesToTop();
                    setSceneItemsMovable(true);
                }
            }
        });
        if (doc)
        {
            if (QUndoStack* stack = activeUndoStack())
            {
                stack->push(new undo::Create(doc.get(),
                                             createdPolyline,
                                             u8"Узел полилинии"));
            }

            _polyline = createdPolyline;

            _drawingPolyline = true;
            _drawingLine = false;
            _resumeEditing = false;
            _resumeUid = 0;

            _isInDrawingMode = true;
            setSceneItemsMovable(false);
            updateModeLabel();

            if (!doc->isModified)
            {
                doc->isModified = true;
                updateFileListDisplay(doc->filePath);
            }
        }
    }
    const QPointF p = _startPoint;

    if (_polyline
        && !_polyline->isClosed()
        && qgraph::Polyline::globalCloseMode() == qgraph::Polyline::CloseMode::SingleClickOnFirstPoint)
    {
        const QVector<qgraph::DragCircle*>& circles = _polyline->circles();

        if (!circles.isEmpty())
        {
            qgraph::DragCircle* firstHandle = circles.first();
            qgraph::DragCircle* clickedHandle = pickHandleAt(p);

            if (clickedHandle && clickedHandle == firstHandle)
            {
                _polyline->closePolyline();

                if (_polyline)
                    apply_PointSize_ToItem(_polyline);

                raiseAllHandlesToTop();

                return;
            }
        }
    }

    if (!_polyline->_circles.isEmpty() &&
        QLineF(_polyline->_circles.last()->scenePos(), p).length() < 0.1)
    {
        // Ничего
    }
    else if (_resumeEditing)
    {
        const int pointIndex = _polyline->circles().size();

        _polyline->addPoint(p, scene);

        if (_polyline->circles().size() > pointIndex)
        {
            _polyline->setFlag(QGraphicsItem::ItemIsMovable, false);
            _polyline->setSelected(true);
            _polyline->setFocus();
            _polyline->updatePointNumbers();

            if (QUndoStack* stack = activeUndoStack())
            {
                undo::NodeEdit::Data data;
                data.type = undo::NodeEdit::Type::InsertPolylineNode;
                data.pointIndex = pointIndex;
                data.point = p;

                stack->push(new undo::NodeEdit(doc.get(),
                                               _polyline,
                                               data,
                                               u8"Узел полилинии"));
            }

            if (doc && !doc->isModified)
            {
                doc->isModified = true;
                updateFileListDisplay(doc->filePath);
            }
        }
    }
    else
    {
        qgraph::Polyline* activePolyline = _polyline;

        const int pointIndex = activePolyline->circles().size();

        _polyline->addPoint(p, scene);

        if (activePolyline->circles().size() > pointIndex)
        {
            activePolyline->setFlag(QGraphicsItem::ItemIsMovable, false);
            activePolyline->setSelected(true);
            activePolyline->setFocus();
            activePolyline->updatePointNumbers();

            if (doc)
            {
                if (QUndoStack* stack = activeUndoStack())
                {
                    undo::NodeEdit::Data data;
                    data.type = undo::NodeEdit::Type::InsertPolylineNode;
                    data.pointIndex = pointIndex;
                    data.point = p;

                    stack->push(new undo::NodeEdit(doc.get(),
                                                   activePolyline,
                                                   data,
                                                   u8"Узел полилинии"));
                }

                _polyline = activePolyline;

                _drawingPolyline = true;
                _drawingLine = false;
                _resumeEditing = false;
                _resumeUid = 0;

                _isInDrawingMode = true;
                setSceneItemsMovable(false);
                updateModeLabel();

                if (!doc->isModified)
                {
                    doc->isModified = true;
                    updateFileListDisplay(doc->filePath);
                }
            }
        }
    }
    bool finishPolyline = false;

    if (qgraph::Polyline::globalCloseMode() == qgraph::Polyline::CloseMode::CtrlModifier
        && (closeMods & Qt::ControlModifier))
    {
        finishPolyline = true;
    }

    if (finishPolyline && _polyline && !_polyline->isClosed())
        _polyline->closePolyline();

    if (_polyline)
        apply_PointSize_ToItem(_polyline);
    raiseAllHandlesToTop();
}

void MainWindow::handleLineLmbClick(const QPointF& scenePos, Qt::KeyboardModifiers mods, GraphicsView* graphView)
{
    //Q_UNUSED(mods);
    Q_UNUSED(graphView);

    const Qt::KeyboardModifiers closeMods = mods | QApplication::keyboardModifiers();

    Document::Ptr doc = currentDocument();
    QGraphicsScene* scene = doc->scene;

     _isInDrawingMode = true;
    setSceneItemsMovable(false);
    updateModeLabel();
    _startPoint = scenePos;
    if (!_line)
    {
        _resumeEditing = false;
        _resumeUid = 0;
        // Если линия еще не создана, создаем объект
        _line = new qgraph::Line(scene, _startPoint);
        qgraph::Line* createdLine = _line;
        ensureUid(createdLine);
        apply_LineWidth_ToItem(createdLine);
        apply_PointSize_ToItem(createdLine);
        apply_NumberSize_ToItem(createdLine);

        createdLine->setSelected(true);
        createdLine->setFocus();
        createdLine->updatePointNumbers();

        createdLine->setModificationCallback([this]()
        {
            if (_drawingLine && _line && _line->isClosed())
            {
                _drawingLine = false;
                _isInDrawingMode = false;
                setSceneItemsMovable(true);
                updateModeLabel();

                _line->updatePointNumbers();
                apply_LineWidth_ToItem(_line);
                apply_PointSize_ToItem(_line);
                apply_NumberSize_ToItem(_line);

                if (!_resumeEditing)
                {
                    SelectClass dialog(_projectClasses, this);

                    if (dialog.exec() == QDialog::Accepted)
                    {
                        const QString selectedClass = dialog.selectedClass();

                        if (!selectedClass.isEmpty())
                        {
                            _line->setClosed(true, false);
                            _line->setFlag(QGraphicsItem::ItemIsMovable, true);

                            _line->setData(0, selectedClass);
                            applyClassColorToItem(_line, selectedClass);
                            linkSceneItemToList(_line);

                            Document::Ptr doc = currentDocument();

                            if (doc)
                            {
                                if (QUndoStack* stack = activeUndoStack())
                                {
                                    stack->push(new undo::FinishDrawing(doc.get(),
                                                                        _line,
                                                                        undo::FinishDrawing::Type::Line,
                                                                        u8"Добавление линии"));
                                }

                                if (!doc->isModified)
                                {
                                    doc->isModified = true;
                                    updateFileListDisplay(doc->filePath);
                                }
                            }

                            _drawingLine = false;
                            _resumeEditing = false;
                            _resumeUid = 0;
                            _line = nullptr;

                            _isInDrawingMode = false;
                            setSceneItemsMovable(true);

                            updateModeLabel();
                            raiseAllHandlesToTop();
                        }
                    }
                }
                else
                {
                    _line->setClosed(true, false);
                    _line->setFlag(QGraphicsItem::ItemIsMovable, true);

                    const QString className = _line->data(0).toString();

                    if (!className.isEmpty())
                        applyClassColorToItem(_line, className);

                    _line->updatePointNumbers();
                    apply_LineWidth_ToItem(_line);
                    apply_PointSize_ToItem(_line);
                    apply_NumberSize_ToItem(_line);

                    Document::Ptr doc = currentDocument();

                    if (doc)
                    {
                        if (QUndoStack* stack = activeUndoStack())
                        {
                            undo::ResumeEdit::Data data;
                            data.type = undo::ResumeEdit::Type::Line;
                            data.resumeIndex = -1;
                            data.removedPoints.clear();
                            data.addedPoints.clear();

                            data.closedBefore = false;
                            data.closedAfter = true;

                            stack->push(new undo::ResumeEdit(doc.get(),
                                                             _line,
                                                             data,
                                                             u8"Завершение продолжения линии"));
                        }

                        if (!doc->isModified)
                        {
                            doc->isModified = true;
                            updateFileListDisplay(doc->filePath);
                        }
                    }

                    _drawingLine = false;
                    _resumeEditing = false;
                    _resumeUid = 0;
                    _line = nullptr;

                    _isInDrawingMode = false;
                    setSceneItemsMovable(true);

                    updateModeLabel();
                    raiseAllHandlesToTop();
                }
            }
        });
        if (doc)
        {
            if (QUndoStack* stack = activeUndoStack())
            {
                stack->push(new undo::Create(doc.get(),
                                             createdLine,
                                             u8"Узел линии"));
            }
            _line = createdLine;

            _drawingLine = true;
            _drawingPolyline = false;
            _resumeEditing = false;
            _resumeUid = 0;

            _isInDrawingMode = true;
            setSceneItemsMovable(false);
            updateModeLabel();

            if (!doc->isModified)
            {
                doc->isModified = true;
                updateFileListDisplay(doc->filePath);
            }
        }
    }
    const QPointF p = _startPoint;
    if (!_line->_circles.isEmpty() &&
        QLineF(_line->_circles.last()->scenePos(), p).length() < 0.1)
    {
        // Ничего
    }
    else if (_resumeEditing)
    {
        const int pointIndex = _line->circles().size();

        _line->addPoint(p, scene);

        if (_line->circles().size() > pointIndex)
        {
            _line->setSelected(true);
            _line->setFocus();
            _line->updatePointNumbers();

            if (QUndoStack* stack = activeUndoStack())
            {
                undo::NodeEdit::Data data;
                data.type = undo::NodeEdit::Type::InsertLineNode;
                data.pointIndex = pointIndex;
                data.point = p;

                stack->push(new undo::NodeEdit(doc.get(),
                                               _line,
                                               data,
                                               u8"Узел линии"));
            }

            if (doc && !doc->isModified)
            {
                doc->isModified = true;
                updateFileListDisplay(doc->filePath);
            }
        }
    }
    else
    {
        qgraph::Line* activeLine = _line;

        const int pointIndex = activeLine->circles().size();

        activeLine->addPoint(p, scene);

        if (activeLine->circles().size() > pointIndex)
        {
            activeLine->setSelected(true);
            activeLine->setFocus();
            activeLine->updatePointNumbers();

            if (doc)
            {
                if (QUndoStack* stack = activeUndoStack())
                {
                    undo::NodeEdit::Data data;
                    data.type = undo::NodeEdit::Type::InsertLineNode;
                    data.pointIndex = pointIndex;
                    data.point = p;

                    stack->push(new undo::NodeEdit(doc.get(),
                                                   activeLine,
                                                   data,
                                                   u8"Узел линии"));
                }

                _line = activeLine;

                _drawingLine = true;
                _drawingPolyline = false;
                _resumeEditing = false;
                _resumeUid = 0;

                _isInDrawingMode = true;
                setSceneItemsMovable(false);
                updateModeLabel();

                if (!doc->isModified)
                {
                    doc->isModified = true;
                    updateFileListDisplay(doc->filePath);
                }
            }
        }
    }
    bool finishLine = false;

    if (qgraph::Line::globalCloseMode() == qgraph::Line::CloseMode::CtrlModifier
        && (closeMods & Qt::ControlModifier))
    {
        finishLine = true;
    }

    if (finishLine && _line && !_line->isClosed())
        _line->closeLine();

    if (_line)
        apply_PointSize_ToItem(_line);
    raiseAllHandlesToTop();
}

void MainWindow::moveCurrentShapeInList(int direction)
{
    if (!ui || !ui->polygonList || direction == 0)
        return;

    Document::Ptr doc = currentDocument();

    if (!doc)
        return;

    QItemSelectionModel* selectionModel = ui->polygonList->selectionModel();

    if (!selectionModel)
        return;

    const QModelIndexList selectedRows = selectionModel->selectedRows();

    if (selectedRows.size() != 1)
        return;

    const int fromRow = selectedRows.first().row();
    const int toRow = fromRow + direction;

    if (fromRow < 0 || toRow < 0 || toRow >= doc->polygonList.items.size())
        return;

    QGraphicsItem* sceneItem = sceneItemFromListRow(fromRow);

    if (!sceneItem)
        return;

    QUndoStack* stack = activeUndoStack();

    if (!stack)
        return;

    stack->push(new undo::ListOrder(doc.get(),
                                    sceneItem,
                                    fromRow,
                                    toRow,
                                    u8"Перемещение фигуры в списке"));

    refreshShapeListOrderRole();

    const int movedRow = polygonListRowByItem(sceneItem);

    if (movedRow >= 0)
    {
        const QModelIndex movedIndex = doc->polygonList.model.index(movedRow, 0);

        ui->polygonList->clearSelection();
        ui->polygonList->setCurrentIndex(movedIndex);

        if (QItemSelectionModel* selectionModel = ui->polygonList->selectionModel())
        {
            selectionModel->select(movedIndex,
                                   QItemSelectionModel::Select
                                   | QItemSelectionModel::Rows);
        }
    }

    if (!doc->isModified)
    {
        doc->isModified = true;
        updateFileListDisplay(doc->filePath);
    }

    onPolygonListSelectionChanged();
    updateShapeListButtons();

    if (doc->scene)
        doc->scene->update();
}

void MainWindow::movePolygonListRow(int fromRow, int toRow)
{
    Document::Ptr doc = currentDocument();
    if (!doc) return;

    int count = doc->polygonList.items.count();

    if (!lst::inRange(fromRow, 0, count))
        return;

    if (!lst::inRange(toRow, 0, count))
        return;

    if (fromRow == toRow)
        return;

    {
        QSignalBlocker blocker {ui->polygonList}; (void) blocker;

        QGraphicsItem* movedSceneItem = doc->polygonList.items.takeAt(fromRow);
        doc->polygonList.items.insert(toRow, movedSceneItem);

        QList<QStandardItem*> movedRow = doc->polygonList.model.takeRow(fromRow);
        doc->polygonList.model.insertRow(toRow, movedRow);

        ui->polygonList->clearSelection();

        QModelIndex movedIndex = doc->polygonList.model.index(toRow, 0);
        ui->polygonList->setCurrentIndex(movedIndex);

        if (QItemSelectionModel* selectionModel = ui->polygonList->selectionModel())
        {
            selectionModel->select(movedIndex, QItemSelectionModel::Select
                                               |QItemSelectionModel::Rows);
        }
    }

    refreshShapeListOrderRole();

    onPolygonListSelectionChanged();
    updateShapeListButtons();

    doc->scene->update();
}

void MainWindow::updateShapeListButtons()
{
    if (!ui || !ui->polygonList)
        return;

    QItemSelectionModel* selectionModel = ui->polygonList->selectionModel();

    const QModelIndexList selectedRows =
        selectionModel ? selectionModel->selectedRows() : QModelIndexList();

    int row = -1;

    if (selectedRows.size() == 1)
        row = selectedRows.first().row();

    Document::Ptr doc = currentDocument();
    const int count = doc ? doc->polygonList.items.size() : 0;

    ui->btnMoveShapeUp->setEnabled(row > 0);
    ui->btnMoveShapeDown->setEnabled(row >= 0 && row < count - 1);
    ui->btnDelete->setEnabled(!selectedRows.isEmpty());
}

void MainWindow::refreshShapeListOrderRole()
{
    Document::Ptr doc = currentDocument();

    if (!doc)
        return;

    for (int row = 0; row < doc->polygonList.items.size(); ++row)
    {
        QGraphicsItem* sceneItem = doc->polygonList.items[row];

        if (!sceneItem)
            continue;

        sceneItem->setData(_roleListOrder, row);
    }
}

int MainWindow::nextShapeNumberForScene(QGraphicsScene* scene) const
{
    if (!scene)
        return 0;

    int maxNumber = -1;

    for (QGraphicsItem* item : scene->items())
    {
        if (!isRootShapeItem(item))
            continue;

        bool ok = false;
        const int number = item->data(_roleShapeNumber).toInt(&ok);

        if (ok && number >= 0)
            maxNumber = std::max(maxNumber, number);
    }

    return maxNumber + 1;
}

int MainWindow::ensureShapeNumber(QGraphicsItem* item) const
{
    if (!item)
        return -1;

    bool ok = false;
    const int number = item->data(_roleShapeNumber).toInt(&ok);

    if (ok && number >= 0)
        return number;

    const int newNumber = nextShapeNumberForScene(item->scene());
    item->setData(_roleShapeNumber, newNumber);

    return newNumber;
}

QList<QGraphicsItem*> MainWindow::orderedShapeItemsForSave(Document::Ptr doc) const
{
    QList<QGraphicsItem*> result;

    if (!doc || !doc->scene)
        return result;

    if (!doc->polygonList.items.isEmpty())
    {
        for (QGraphicsItem* sceneItem : doc->polygonList.items)
        {
            if (!sceneItem)
                continue;

            if (sceneItem->scene() != doc->scene)
                continue;

            if (!isRootShapeItem(sceneItem))
                continue;

            result.append(sceneItem);
        }

        return result;
    }

    for (QGraphicsItem* item : doc->scene->items())
    {
        if (isRootShapeItem(item))
            result.append(item);
    }

    return result;
}

void MainWindow::syncZValuesWithListOrder(Document::Ptr doc)
{
    if (!doc || !doc->scene)
        return;

    const QList<QGraphicsItem*> items = orderedShapeItemsForSave(doc);

    const qreal baseZ = doc->videoRect ? doc->videoRect->zValue() : 0.0;
    qreal z = baseZ + 1.0;

    for (QGraphicsItem* item : items)
    {
        if (!item)
            continue;

        item->setZValue(z++);
    }

    raiseAllHandlesToTop();

    if (doc->scene)
        doc->scene->update();
}
