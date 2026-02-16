#include "main_window.h"
#include "ui_main_window.h"
//#include "event_window.h"

#include "shared/defmac.h"
#include "shared/break_point.h"
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
#include <QSlider>

#include "select_class.h"
#include "settings.h"
#include "user_guide.h"

#include <QApplication>
#include <QHostInfo>
#include <QScreen>
#include <QLabel>
#include <QPointF>
#include <QFileInfo>
#include <QInputDialog>
#include <QGraphicsItem>
#include <QVBoxLayout>
#include <QPixmap>
#include <QFrame>
#include <QMouseEvent>
#include <QScrollBar>
#include <QWheelEvent>
#include <QEvent>
#include <QKeyEvent>
#include <unistd.h>
#include <QPen>
#include <QSplitter>
#include <QTextEdit>
#include <QListWidget>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QClipboard>
#include <QMimeData>
#include <QGuiApplication>
#include <cmath>
#include <limits>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QAbstractButton>
#include <QAction>
#include <QMenu>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsRectItem>
#include <QAbstractItemView>
#include <QMetaType>
#include <QtGlobal>
#include <QShortcut>
#include <QKeySequence>
#include <QPainterPathStroker>
#include <QStyleFactory>


using namespace qgraph;
//#include <view.h>

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
                           double minV, double maxV, double step,
                           double value,
                           QWidget* parent=nullptr)
        : QDialog(parent)
    {
        setWindowTitle(title);
        auto *spin = new QDoubleSpinBox; spin->setRange(minV, maxV); spin->setDecimals(1); spin->setSingleStep(step); spin->setValue(value);
        auto *form = new QFormLayout; form->addRow(label, spin);
        auto *btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        QObject::connect(btns, &QDialogButtonBox::accepted, this, &QDialog::accept);
        QObject::connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
        auto *box = new QVBoxLayout(this); box->addLayout(form); box->addWidget(btns);
        _spin = spin;
    }
    double value() const { return _spin->value(); }
private:
    QDoubleSpinBox* _spin = nullptr;
};

// Идем вверх по родителям, ищем перемещаемого владельца
static QGraphicsItem* findMovableAncestor(QGraphicsItem* it)
{
    for (QGraphicsItem* p = it; p; p = p->parentItem())
    {
        if (p->flags().testFlag(QGraphicsItem::ItemIsMovable))
            return p;
    }
    return it;
}

// Простое сравнение геометрии снапшотов, чтобы не совершать no-op команды
static bool sameGeometry(const ShapeBackup& a, const ShapeBackup& b)
{
    if (a.kind != b.kind)
        return false;

    switch (a.kind)
    {
    case ShapeKind::Rectangle:
        return a.rect == b.rect && a.z == b.z && a.visible == b.visible && a.className == b.className;

    case ShapeKind::Circle:
        return a.circleCenter == b.circleCenter
            && a.circleRadius == b.circleRadius
            && a.z == b.z && a.visible == b.visible && a.className == b.className;

    case ShapeKind::Polyline:
        return a.points == b.points
            && a.closed == b.closed
            && a.z == b.z && a.visible == b.visible && a.className == b.className;

    case ShapeKind::Line:
        return a.points == b.points
            && a.z == b.z && a.visible == b.visible && a.className == b.className;

    case ShapeKind::Point:
        return a.pointCenter == b.pointCenter
            && a.z == b.z && a.visible == b.visible && a.className == b.className;
    }
    return false;
}


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    _scene(new QGraphicsScene(this))
    //_socket(new tcp::Socket)
{
    ui->setupUi(this);

    //ui->menuBar->setAttribute(Qt::WA_TransparentForMouseEvents, false);

    ui->graphView->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    ui->graphView->setCacheMode(QGraphicsView::CacheNone);

    // ui->toolBar->setAllowedAreas(Qt::TopToolBarArea);
    // ui->toolBar->setMovable(false);
    // ui->toolBar->setFloatable(false);
    // addToolBarBreak(Qt::TopToolBarArea);

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
    auto* htb = new QHBoxLayout(toolBarsBlock);
    htb->setContentsMargins(0, 0, 0, 0);
    htb->setSpacing(6); // Расстояние между тулбарами

    // Чтобы первый тулбар не растягивался, а второй занимал остаток
    ui->toolBar->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    ui->toolBar_2->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // Выравниваем по центру по вертикали
    htb->addWidget(ui->toolBar, 0, Qt::AlignVCenter);
    htb->addWidget(ui->toolBar_2,   1, Qt::AlignVCenter);

    htb->setStretch(0, 0); // toolBar_2 не растягиваем
    htb->setStretch(1, 1); // toolBar растягиваем
    ui->toolBar->setMinimumWidth(ui->toolBar->sizeHint().width());

    // Убираем старые виджеты из сетки и вставляем блок тулбаров в row 0
    ui->gridLayout->removeWidget(ui->label_5);
    ui->gridLayout->removeWidget(ui->label);
    ui->gridLayout->removeWidget(ui->label_4);
    ui->gridLayout->removeWidget(ui->polygonList);
    ui->gridLayout->removeWidget(ui->coordinateList);
    ui->gridLayout->removeWidget(ui->undoView);

    // Добавляем контейнер toolBarsBlock
    ui->gridLayout->addWidget(toolBarsBlock, 0, 0, 1, 3);

    // Возвращаем заголовки и списки ниже
    ui->gridLayout->addWidget(ui->label_5, 1, 0);
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
    auto fixSelectionPalette = [](QListWidget* w){
        QPalette pal = w->palette();
        const QColor hiA  = pal.color(QPalette::Active,   QPalette::Highlight);
        const QColor txtA = pal.color(QPalette::Active,   QPalette::HighlightedText);

        pal.setColor(QPalette::Inactive, QPalette::Highlight,       hiA);
        pal.setColor(QPalette::Inactive, QPalette::HighlightedText, txtA);
        pal.setColor(QPalette::Disabled, QPalette::Highlight,       hiA);
        pal.setColor(QPalette::Disabled, QPalette::HighlightedText, txtA);

        w->setPalette(pal);
        if (w->viewport()) w->viewport()->setPalette(pal);
    };

    fixSelectionPalette(ui->polygonList);
    // fixSelectionPalette(ui->listWidget_Classes);
    fixSelectionPalette(ui->fileList);


    qRegisterMetaType<QGraphicsItem*>("QGraphicsItem*");

    if (_scene) _scene->installEventFilter(this);
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

    ui->graphView->setScene(_scene);
    connect(_scene, &QGraphicsScene::changed,
            this,  &MainWindow::onSceneChanged);

    _videoRect = new qgraph::VideoRect(_scene);

    //_labelConnectStatus = new QLabel(u8"Нет подключения", this);
    //ui->statusBar->addWidget(_labelConnectStatus);

    ui->graphView->viewport()->installEventFilter(this);
    ui->graphView->setMouseTracking(true);    

    // Создаем и настраиваем метки
    // _folderPathLabel = new QLabel(this);
    // _folderPathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    // _folderPathLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    // ui->statusBar->addWidget(_folderPathLabel, 1); // Растягиваемый

    QString vers = u8"Версия: %1 (gitrev: %2)";
    vers = vers.arg(VERSION_PROJECT).arg(GIT_REVISION);
    ui->statusBar->addPermanentWidget(new QLabel(vers, this));

    _imageSizeLabel = new QLabel(this);
    _imageSizeLabel->setText(tr("Размер изображения: —"));
    _imageSizeLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    statusBar()->addWidget(_imageSizeLabel, 1);

    QString fileName = "/home/marie/фон.png";
    QPixmap pix(fileName);
    _videoRect->setPixmap(pix);

    // Масштабируем фон под размер окна
    pix = pix.scaled(ui->graphView->size(), Qt::KeepAspectRatioByExpanding);
    // Устанавливаем изображение фоном для сцены
    ui->graphView->setBackgroundBrush(pix);


    // Загружаем файлы из папки при старте
    //loadFilesFromFolder("/home/marie/тестовые данные QLabelMe/фото");  // Укажите нужный путь
    //loadFilesFromFolder("/home/hkarel/Images/1");  // Укажите нужный путь
    // Загрузка данных из файла при запуске
    //loadPolygonLabelsFromFile("/home/marie/тестовые данные QLabelMe/список классов/список классов.txt"); // Укажите актуальный путь к файлу
    //loadPolygonLabelsFromFile("/home/hkarel/Images/1/список классов.txt"); // Укажите актуальный путь к файлу
    // Инициализация с пустой папкой
    _currentFolderPath = "";


    // TODO все коннекты исправить на chk_connect_a
    chk_connect_a(ui->fileList, &QListWidget::currentItemChanged,
                  this, &MainWindow::fileList_ItemChanged);

    connect(ui->polygonList, &QListWidget::itemClicked,
            this, &MainWindow::onPolygonListItemClicked);
    connect(ui->polygonList, &QListWidget::itemDoubleClicked,
            this, &MainWindow::onPolygonListItemDoubleClicked);
    connect(_scene, &QGraphicsScene::selectionChanged,
            this, &MainWindow::onSceneSelectionChanged);
    connect(_scene, &QGraphicsScene::changed,
            this,  &MainWindow::onSceneChanged);
    connect(ui->polygonList, &QListWidget::itemSelectionChanged,
            this, &MainWindow::onPolygonListSelectionChanged);


    auto shNext = new QShortcut(QKeySequence(Qt::Key_D), this);
    shNext->setContext(Qt::ApplicationShortcut);
    connect(shNext, &QShortcut::activated, this, &MainWindow::nextImage);

    auto shPrev = new QShortcut(QKeySequence(Qt::Key_A), this);
    shPrev->setContext(Qt::ApplicationShortcut);
    connect(shPrev, &QShortcut::activated, this, &MainWindow::prevImage);

    ui->polygonList->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->polygonList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    connect(ui->polygonList, &QListWidget::customContextMenuRequested,
            this, &MainWindow::showPolygonListContextMenu);

    //QLabel* pathHeader = new QLabel(this);
    //pathHeader->setAlignment(Qt::AlignCenter);
    // Добавляем заголовок над listWidget
    //ui->verticalLayout->insertWidget(0, pathHeader);


    ui->graphView->viewport()->setMouseTracking(true);
    ui->graphView->setMouseTracking(true);

    _scene->installEventFilter(this);

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
    _rightPanel = ui->splitter->widget(1); // Или другой индекс, в зависимости от структуры
    // Сохраняем начальные размеры сплиттера
    _savedSplitterSizes = ui->splitter->sizes();
    applyLabelFontToUi();

    // сцена должна отдавать нам события
    _scene->installEventFilter(this);

    // нужен свободный трекинг мыши
    ui->graphView->viewport()->setMouseTracking(true);

    _lastHoverHandle = nullptr;

    int cmPolyline = static_cast<int>(Settings::PolylineCloseMode::CtrlModifier);
    config::base().getValue("polyline.close_mode", cmPolyline);
    _polylineCloseMode = static_cast<Settings::PolylineCloseMode>(cmPolyline);
    applyClosePolyline();

    int cmLine = static_cast<int>(Settings::LineFinishMode::CtrlModifier);
    config::base().getValue("line.finish_mode", cmLine);
    _lineFinishMode = static_cast<Settings::LineFinishMode>(cmLine);
    applyFinishLine();

    ensureGhostAllocated();

    //config::base().getValue("view.keep_image_scale", init.keepImageScale);



    // Стек и действия
    // _undoStack = std::make_unique<QUndoStack>(this);
    // _actUndo = _undoStack->createUndoAction(this, tr("Отменить"));
    // _actRedo = _undoStack->createRedoAction(this, tr("Повторить"));

    // Группа стеков действий для разных документов
    _undoGroup = new QUndoGroup(this);

    // _actUndo = _undoGroup->createUndoAction(this, tr("Отменить"));
    // _actRedo = _undoGroup->createRedoAction(this, tr("Повторить"));

    // _actUndo->setShortcut(QKeySequence::Undo);
    // _actRedo->setShortcuts({QKeySequence::Redo, QKeySequence(Qt::CTRL | Qt::Key_Y)});

    // _actUndo->setShortcutContext(Qt::ApplicationShortcut);
    // _actRedo->setShortcutContext(Qt::ApplicationShortcut);

    // Фиксированные подписи без динамического хвоста
    // _actUndo = new QAction(tr("Отменить"), this);
    // _actRedo = new QAction(tr("Повторить"), this);

    // Триггеры - методы группы
    // connect(_actUndo, &QAction::triggered, _undoGroup, &QUndoGroup::undo);
    // connect(_actRedo, &QAction::triggered, _undoGroup, &QUndoGroup::redo);

    // Включаемость по состоянию группы
    connect(_undoGroup, &QUndoGroup::canUndoChanged, ui->actUndo, &QAction::setEnabled);
    connect(_undoGroup, &QUndoGroup::canRedoChanged, ui->actRedo, &QAction::setEnabled);

    // Начальное состояние
    ui->actUndo->setEnabled(_undoGroup->canUndo());
    ui->actRedo->setEnabled(_undoGroup->canRedo());

    // _actUndo->setShortcut(QKeySequence::Undo);
    // _actRedo->setShortcuts({QKeySequence::Redo, QKeySequence(Qt::CTRL | Qt::Key_Y)});

    ui->actUndo->setShortcuts(QKeySequence::keyBindings(QKeySequence::Undo));
    ui->actRedo->setShortcuts(QKeySequence::keyBindings(QKeySequence::Redo));

    ui->actUndo->setShortcutContext(Qt::ApplicationShortcut);
    ui->actRedo->setShortcutContext(Qt::ApplicationShortcut);

    // _actUndo->setToolTip(tr("Отменить"));
    // _actRedo->setToolTip(tr("Повторить"));

    // actUndo->setIcon(style()->standardIcon(QStyle::SP_ArrowBack));
    // actRedo->setIcon(style()->standardIcon(QStyle::SP_ArrowForward));

    //ui->toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);


    // addAction(actUndo);
    // addAction(actRedo);

    // Добавление в меню/тулбар
    // ui->menuEdit->addAction(_actUndo);
    // ui->menuEdit->addAction(_actRedo);
    // if (ui->toolBar)
    // {
    //     ui->toolBar->addAction(_actUndo);
    //     ui->toolBar->addAction(_actRedo);
    // }

    // auto undoBtn = qobject_cast<QToolButton*>(
    //     ui->toolBar->widgetForAction(_actUndo)
    // );
    // if (undoBtn)
    // {
    //     undoBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    // }

    // auto redoBtn = qobject_cast<QToolButton*>(
    //     ui->toolBar->widgetForAction(_actRedo)
    // );
    // if (redoBtn)
    // {
    //     redoBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    // }

    ui->toolBar->setIconSize(QSize(32, 32));

    // connect(_undoStack.get(), &QUndoStack::indexChanged, this, [this]() {
    //     if (_loadingNow)
    //         return;
    //     auto doc = currentDocument();
    //     if (!doc)
    //         return;

    //     doc->isModified = true;
    //     updateFileListDisplay(doc->filePath);
    // });

    // Панель с QUndoView для визуального контроля
    // _undoView = new QUndoView(_undoStack.get(), this);
    // _undoView->setEmptyLabel(tr("<пусто>"));
    // _undoDock = new QDockWidget(tr("Стек действий (временный)"), this);
    // _undoDock->setObjectName("UndoDockDebug");
    // _undoDock->setWidget(_undoView);
    // addDockWidget(Qt::RightDockWidgetArea, _undoDock);

    _undoView = ui->undoView;
    //_undoView->setStack(_undoStack.get());
    _undoView->setEmptyLabel(tr("<пусто>"));

    // Классы в настройках
    _projPropsDialog = new ProjectSettings(this);

    connect(_projPropsDialog, &ProjectSettings::classesApplied, this,
        [this](const QStringList& classes){
            _projectClasses = classes;

            _projectClassColors = _projPropsDialog->projectClassColors();

            if (!saveProjectClasses(_projectClasses, _projectClassColors))
            {
                QMessageBox::warning(this, tr("Ошибка"),
                                     tr("Не удалось сохранить classes.yaml"));
            }
        });

    connect(_projPropsDialog, &ProjectSettings::classColorsApplied, this,
        [this](const QMap<QString, QColor>& colors)
        {
            _projectClassColors = colors;

            for (QGraphicsItem* it : _scene->items())
            {
                const QString cls = it->data(0).toString();
                if (!cls.isEmpty())
                    applyClassColorToItem(it, cls);
            }
            _scene->update();
        });

    connect(ui->actUserGuide, &QAction::triggered, this, [this]()
    {
        UserGuide *guide = new UserGuide(this);
        guide->show();
    });

    // Чтобы Alt работал независимо от фокуса
    qApp->installEventFilter(this);

    // Восстановить видимость меню
    // bool mbVisible = true;
    // ui->menuBar->setVisible(mbVisible);
    bool keepMenuVis = false;
    config::base().getValue("ui.keep_menu_visibility", keepMenuVis);

    bool mbVisible = true; // по умолчанию при запуске меню видно
    if (keepMenuVis)
        config::base().getValue("ui.menu_visible", mbVisible);

    ui->menuBar->setVisible(mbVisible);

    // Чтобы шорткаты работали даже при скрытом menuBar
    const auto acts = findChildren<QAction*>();
    for (QAction* a : acts)
    {
        if (!a)
            continue;

        if (a->menu() != nullptr)
            continue;

        if (a->shortcut().isEmpty())
            continue;

        a->setShortcutContext(Qt::WindowShortcut);
        this->addAction(a);
    }
}

MainWindow::~MainWindow()
{
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
    // Загружаем визуальный стиль
    loadVisualStyle();
    // Применяем загруженный стиль ко всем документам
    applyStyle_AllDocuments();
    return true;
}

void MainWindow::deinit()
{
}

void MainWindow::graphicsView_mousePressEvent(QMouseEvent* mouseEvent, GraphicsView* graphView)
{
    if (mouseEvent->button() == Qt::LeftButton)
        _leftMouseButtonDown = true;

    if (_isInDrawingMode)
    {
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
        Polyline* poly = nullptr;
        const auto itemsAtPos = graphView->scene()->items(scenePos);
        for (QGraphicsItem* it : itemsAtPos) {
            if (auto* p = dynamic_cast<Polyline*>(it)) { poly = p; break; }
        }
        if (!poly) return;

        // Ищем ближайшую точку
        DragCircle* nearest = nullptr;
        qreal bestDist2 = 10.0 * 10.0; // радиус 10 px
        for (DragCircle* c : poly->circles()) {
            const QPointF p = c->scenePos();
            const qreal dx = p.x() - scenePos.x();
            const qreal dy = p.y() - scenePos.y();
            const qreal d2 = dx*dx + dy*dy;
            if (d2 < bestDist2) { bestDist2 = d2; nearest = c; }
        }

        if (nearest)
        {
            nearest->setUserHidden(!nearest->isUserHidden());
        }

        return;
    }
    if (mouseEvent->button() == Qt::LeftButton)
    {
        QGraphicsItem* clickedItem = graphView->itemAt(mouseEvent->pos());

        if (mouseEvent->modifiers() & Qt::ShiftModifier)
        {
            // Shift - двигаем только фото и пишем это в стек
            if (clickedItem == _videoRect && _videoRect)
            {
                _shiftImageBeforePos  = _videoRect->pos();
                _shiftImageDragging   = true;

                // В этом режиме мы не двигаем фигуры
                _isDraggingImage = false;
                _draggingItem    = nullptr;

                _lastMousePos = mouseEvent->pos();
                mouseEvent->accept();
                return;
            }

            // При зажатом Shift двигаем именно фигуру, а не ее дочерние элементы
            if (clickedItem && !qgraphicsitem_cast<qgraph::DragCircle*>(clickedItem))
            {
                _draggingItem = findMovableAncestor(clickedItem);
            }
            else
            {
                _draggingItem = nullptr;
            }

            _isDraggingImage = false;
            _shiftImageDragging = false;

            _lastMousePos = mouseEvent->pos();
            mouseEvent->accept();
            return;
        }
        else
        {
            // Без Shift - проверяем, кликнули ли на изображение
            _isDraggingImage = (clickedItem == _videoRect);
            _draggingItem = nullptr;
            _shiftImageDragging = false;
            _isAllMoved = false;
        }

        _lastMousePos = mouseEvent->pos();

    }

    if (mouseEvent->button() == Qt::LeftButton)
    {
        QGraphicsItem* clickedItem = graphView->itemAt(mouseEvent->pos());

        if (clickedItem && !qgraphicsitem_cast<qgraph::DragCircle*>(clickedItem))
        {
            QGraphicsItem* mov = findMovableAncestor(clickedItem);
            if (mov && mov->flags().testFlag(QGraphicsItem::ItemIsMovable))
            {
                _movingItem = mov;
                _moveBeforeSnap = makeBackupFromItem(_movingItem);
                _moveHadChanges = false;

                const QPointF scenePos = graphView->mapToScene(mouseEvent->pos());
                _moveGrabOffsetScene = scenePos - _movingItem->scenePos();
                _moveSavedFlags = _movingItem->flags();
                _movingItem->setFlag(QGraphicsItem::ItemIsMovable, false);

                _movingItems.clear();
                _moveGroupBefore.clear();
                _moveGroupInitialPos.clear();
                _moveIsGroup        = false;
                _movePressScenePos  = scenePos;

                // Если выделено несколько фигур
                const QList<QGraphicsItem*> selectedItems = _scene->selectedItems();
                if (selectedItems.size() > 1 && mov->isSelected())
                {
                    _moveIsGroup = true;

                    for (QGraphicsItem* sel : selectedItems)
                    {
                        if (!sel)
                            continue;

                        QGraphicsItem* root = findMovableAncestor(sel);
                        if (!root)
                            continue;

                        // Избегаем дублей
                        if (_movingItems.contains(root))
                            continue;

                        _movingItems.append(root);
                        _moveGroupBefore.append(makeBackupFromItem(root));
                        _moveGroupInitialPos.append(root->scenePos());
                    }

                    // На всякий случай гарантируем, что "ведущий" item есть в списке
                    if (!_movingItems.contains(_movingItem))
                    {
                        _movingItems.append(_movingItem);
                        _moveGroupBefore.append(makeBackupFromItem(_movingItem));
                        _moveGroupInitialPos.append(_movingItem->scenePos());
                    }
                }
            }
        }
    }

    if (_drawingCircle)
    {
        _isInDrawingMode = true;
        setSceneItemsMovable(false);
        if (!_isDrawingCircle)
        {
            _startPoint = graphView->mapToScene(mouseEvent->pos());
            if (!_currCircle)
            {
                _currCircle = new QGraphicsEllipseItem();
                QPen pen;
                pen.setWidthF(_vis.lineWidth);
                pen.setColor(_vis.circleLineColor);
                pen.setCosmetic(true);
                _currCircle->setPen(pen);
                _currCircle->setBrush(Qt::transparent);

                _scene->addItem(_currCircle);
            }
            // Задаем позицию и начальный размер
            _currCircle->setRect(QRectF(_startPoint, QSizeF(1, 1)));
            _isDrawingCircle = true;

            const qreal crossSize = 10.0;
            if (!_currCircleCrossV)
            {
                _currCircleCrossV = _scene->addLine(0, 0, 0, 0);
                _currCircleCrossV->setFlag(QGraphicsItem::ItemIsMovable, false);
                _currCircleCrossV->setFlag(QGraphicsItem::ItemIsSelectable, false);
                _currCircleCrossV->setFlag(QGraphicsItem::ItemIsFocusable, false);
                _currCircleCrossV->setAcceptedMouseButtons(Qt::NoButton);
            }
            if (!_currCircleCrossH)
            {
                _currCircleCrossH = _scene->addLine(0, 0, 0, 0);
                _currCircleCrossH->setFlag(QGraphicsItem::ItemIsMovable, false);
                _currCircleCrossH->setFlag(QGraphicsItem::ItemIsSelectable, false);
                _currCircleCrossH->setFlag(QGraphicsItem::ItemIsFocusable, false);
                _currCircleCrossH->setAcceptedMouseButtons(Qt::NoButton);
            }

            QPen pen;
            pen.setWidthF(_vis.lineWidth);
            pen.setColor(_vis.circleLineColor);
            pen.setCosmetic(true);
            _currCircleCrossV->setPen(pen);
            _currCircleCrossH->setPen(pen);

            const qreal cx = _startPoint.x();
            const qreal cy = _startPoint.y();

            _currCircleCrossV->setLine(cx, cy - crossSize, cx, cy + crossSize);
            _currCircleCrossH->setLine(cx - crossSize, cy, cx + crossSize, cy);
        }
        else if (_currCircle)
        {
            _isDrawingCircle = false;
        }
        _drawingCircle = false;
    }

    else if (_drawingPolyline)
    {
        _isInDrawingMode = true;
        setSceneItemsMovable(false);
        _startPoint = graphView->mapToScene(mouseEvent->pos());
        if (!_polyline)
        {
            _resumeEditing = false;
            _resumeUid = 0;
            // Если полилиния еще не создана, создаем объект
            _polyline = new qgraph::Polyline(_scene, _startPoint);
            apply_LineWidth_ToItem(_polyline);
            apply_PointSize_ToItem(_polyline);
            apply_NumberSize_ToItem(_polyline);
            _polyline->setFocus();
            _polyline->setModificationCallback([this]()
            {
                if (_drawingPolyline && _polyline && _polyline->isClosed())
                {
                    _drawingPolyline = false;

                    _polyline->updatePointNumbers();
                    apply_LineWidth_ToItem(_polyline);
                    apply_PointSize_ToItem(_polyline);
                    apply_NumberSize_ToItem(_polyline);

                    // QStringList classes;
                    // for (int i = 0; i < ui->polygonLabel->count(); ++i)
                    //     classes << ui->polygonLabel->item(i)->text();

                    // SelectClass dialog(classes, this);
                    if (!_resumeEditing)
                    {
                        SelectClass dialog(_projectClasses, this);

                        if (dialog.exec() == QDialog::Accepted)
                        {
                            const QString selectedClass = dialog.selectedClass();
                            if (!selectedClass.isEmpty())
                            {
                                _polyline->setData(0, selectedClass);
                                applyClassColorToItem(_polyline, selectedClass);
                                linkSceneItemToList(_polyline);

                                ShapeBackup b = makeBackupFromItem(_polyline);
                                pushAdoptExistingShapeCommand(_polyline, b, tr("Добавление полилинии"));
                            }
                        }
                    }

                    _polyline = nullptr;

                    if (auto doc = currentDocument())
                    {
                        doc->isModified = true;
                        updateFileListDisplay(doc->filePath);
                    }
                    raiseAllHandlesToTop();
                }
            });
        }
        _polyline->addPoint(_startPoint, _scene);
        // Закрытие полилинии по Ctrl
        if (qgraph::Polyline::globalCloseMode() == qgraph::Polyline::CloseMode::CtrlModifier &&
            (mouseEvent->modifiers() & Qt::ControlModifier))
        {
            _polyline->closePolyline();
        }
        apply_PointSize_ToItem(_polyline);
        raiseAllHandlesToTop();
    }

    else if (_drawingRectangle)
    {
        _isInDrawingMode = true;
        setSceneItemsMovable(false);
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
                pen.setWidthF(_vis.lineWidth);
                pen.setColor(_vis.rectangleLineColor);
                pen.setCosmetic(true);
                _currRectangle->setPen(pen);
                _currRectangle->setBrush(Qt::transparent);

                _scene->addItem(_currRectangle);
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

        const QPointF sceneP = graphView->mapToScene(mouseEvent->pos());

        _currPoint = new qgraph::Point(_scene);

        // Применяем стили из настроек
        apply_PointSize_ToItem(_currPoint);
        apply_NumberSize_ToItem(_currPoint);
        apply_PointStyle_ToItem(_currPoint);

        _currPoint->setCenter(sceneP);
        _currPoint->setFocus();

        // QStringList classes;
        // for (int i = 0; i < ui->polygonLabel->count(); ++i)
        //     classes << ui->polygonLabel->item(i)->text();

        // SelectClass dialog(classes, this);
        SelectClass dialog(_projectClasses, this);

        if (dialog.exec() == QDialog::Accepted)
        {
            const QString selectedClass = dialog.selectedClass();
            if (!selectedClass.isEmpty())
            {
                _currPoint->setData(0, selectedClass);
                applyClassColorToItem(_currPoint, selectedClass);
                linkSceneItemToList(_currPoint);

                ShapeBackup b = makeBackupFromItem(_currPoint);
                pushAdoptExistingShapeCommand(_currPoint, b, tr("Добавление точки"));


            }
        }

        _currPoint = nullptr;

        if (auto doc = currentDocument())
        {
            doc->isModified = true;
            updateFileListDisplay(doc->filePath);
        }

        // Сразу завершаем (точка - единичный клик)
        _drawingPoint = false;
        _isInDrawingMode = false;
        setSceneItemsMovable(true);

        raiseAllHandlesToTop();
        return;
    }
    else if (_drawingLine)
    {
        _isInDrawingMode = true;
        setSceneItemsMovable(false);
        _startPoint = graphView->mapToScene(mouseEvent->pos());
        if (!_line)
        {
            _resumeEditing = false;
            _resumeUid = 0;
            // Если линия еще не создана, создаем объект
            _line = new qgraph::Line(_scene, _startPoint);
            apply_LineWidth_ToItem(_line);
            apply_PointSize_ToItem(_line);
            apply_NumberSize_ToItem(_line);
            _line->setFocus();
            _line->setModificationCallback([this]()
            {
                if (_drawingLine && _line && _line->isClosed())
                {
                    _drawingLine = false;

                    _line->updatePointNumbers();
                    apply_LineWidth_ToItem(_line);
                    apply_PointSize_ToItem(_line);
                    apply_NumberSize_ToItem(_line);

                    // QStringList classes;
                    // for (int i = 0; i < ui->polygonLabel->count(); ++i)
                    //     classes << ui->polygonLabel->item(i)->text();

                    // SelectClass dialog(classes, this);

                    if (!_resumeEditing)
                    {
                        SelectClass dialog(_projectClasses, this);

                        if (dialog.exec() == QDialog::Accepted)
                        {
                            const QString selectedClass = dialog.selectedClass();
                            if (!selectedClass.isEmpty())
                            {
                                _line->setData(0, selectedClass);
                                applyClassColorToItem(_line, selectedClass);
                                linkSceneItemToList(_line);

                                ShapeBackup b = makeBackupFromItem(_line);
                                pushAdoptExistingShapeCommand(_line, b, tr("Добавление линии"));
                            }
                        }
                    }
                    _line = nullptr;

                    if (auto doc = currentDocument())
                    {
                        doc->isModified = true;
                        updateFileListDisplay(doc->filePath);
                    }
                    raiseAllHandlesToTop();
                }
            });
        }
        _line->addPoint(_startPoint, _scene);
        // Завершение линии по Ctrl
        if (qgraph::Line::globalCloseMode() == qgraph::Line::CloseMode::CtrlModifier &&
            (mouseEvent->modifiers() & Qt::ControlModifier))
        {
            _line->closeLine();
        }
        apply_PointSize_ToItem(_line);
        raiseAllHandlesToTop();
    }
    else if (_drawingRuler)
    {
        //_isInDrawingMode = true;
        //setSceneItemsMovable(false);

        if (!_isDrawingRuler)
        {
            // Первая точка линейки
            _rulerStartPoint = graphView->mapToScene(mouseEvent->pos());
            _isDrawingRuler  = true;

            // Удаляем старую линейку (если осталась от прошлого измерения)
            if (_rulerLine)
            {
                _scene->removeItem(_rulerLine);
                delete _rulerLine;
                _rulerLine = nullptr;
            }
            if (_rulerText)
            {
                _scene->removeItem(_rulerText);
                delete _rulerText;
                _rulerText = nullptr;
            }

            QPen pen(QColor(200, 200, 200));
            pen.setWidthF(2.0);
            pen.setStyle(Qt::DotLine);
            pen.setCapStyle(Qt::RoundCap);
            pen.setJoinStyle(Qt::RoundJoin);
            pen.setCosmetic(true);

            _rulerLine = _scene->addLine(QLineF(_rulerStartPoint, _rulerStartPoint), pen);
            _rulerLine->setZValue(1000);

            // Текст с расстоянием
            _rulerText = _scene->addText(QStringLiteral("0"));
            _rulerText->setDefaultTextColor(QColor(200, 200, 200));
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
        }
        return;
    }
    if (_movingItem)
    {
        const QPointF scenePos = graphView->mapToScene(mouseEvent->pos());

        if (_moveIsGroup && !_movingItems.isEmpty())
        {
            // Сдвиг относительно положения курсора при захвате
            const QPointF shift = scenePos - _movePressScenePos;

            const int count = qMin(_movingItems.size(), _moveGroupInitialPos.size());
            for (int i = 0; i < count; ++i)
            {
                QGraphicsItem* item = _movingItems[i];
                if (!item)
                    continue;

                const QPointF& startPos = _moveGroupInitialPos[i];
                item->setPos(startPos + shift);
            }
        }
        else
        {
            _movingItem->setPos(scenePos - _moveGrabOffsetScene);
        }

        _moveHadChanges = true;
    }

    if (mouseEvent->buttons() & Qt::LeftButton)
    {
        QPointF delta = graphView->mapToScene(mouseEvent->pos()) -
                        graphView->mapToScene(_lastMousePos);

        // Shift-перетаскивание только фото
        if (_shiftImageDragging)
        {
            if (_videoRect)
                _videoRect->moveBy(delta.x(), delta.y());

            updateAllPointNumbers();
            _scene->update();
            ui->graphView->viewport()->update();

            _lastMousePos = mouseEvent->pos();
            mouseEvent->accept();
            return; // чтобы не сработал _isDraggingImage ниже
        }

        if (_isDraggingImage)
        {
            if (_videoRect)
                _videoRect->moveBy(delta.x(), delta.y());

            // Перемещаем все фигуры вместе с фото
            for (QGraphicsItem* item : _scene->items())
            {
                if (!item)
                    continue;

                if (item == _videoRect)
                    continue;

                if (dynamic_cast<qgraph::DragCircle*>(item))
                    continue;

                if (item->parentItem() != nullptr)
                    continue;

                item->moveBy(delta.x(), delta.y());

                if (auto* circle = dynamic_cast<qgraph::Circle*>(item))
                    circle->updateHandlePosition();
                else if (auto* rect = dynamic_cast<qgraph::Rectangle*>(item))
                    rect->updateHandlePosition();
                else if (auto* polyline = dynamic_cast<qgraph::Polyline*>(item))
                    polyline->updateHandlePosition();
            }
            updateAllPointNumbers();
            _scene->update();
            ui->graphView->viewport()->update();
            _isAllMoved = true;
        }
        else if (_draggingItem && (mouseEvent->modifiers() & Qt::ShiftModifier))
        {
            // Смещение в системе координат сцены
            const QPointF curScene = graphView->mapToScene(mouseEvent->pos());
            const QPointF prevScene = graphView->mapToScene(_lastMousePos);
            const QPointF delta = curScene - prevScene;

            _draggingItem->moveBy(delta.x(), delta.y());

            // Обновляем ручки для перемещаемой фигуры
            if (auto* circle = dynamic_cast<qgraph::Circle*>(_draggingItem))
                circle->updateHandlePosition();
            else if (auto* rect = dynamic_cast<qgraph::Rectangle*>(_draggingItem))
                rect->updateHandlePosition();
            else if (auto* polyline = dynamic_cast<qgraph::Polyline*>(_draggingItem))
                polyline->updateHandlePosition();
            updateAllPointNumbers();
        }

        _lastMousePos = mouseEvent->pos();
    }
    _lastMousePos = mouseEvent->pos();
    updateAllPointNumbers();
}

void MainWindow::graphicsView_mouseReleaseEvent(QMouseEvent* mouseEvent, GraphicsView* graphView)
{
    if (mouseEvent->button() == Qt::LeftButton)
        _leftMouseButtonDown = false;

    if (_shiftImageDragging && mouseEvent->button() == Qt::LeftButton)
    {
        _shiftImageDragging = false;

        if (_videoRect)
        {
            QPointF after = _videoRect->pos();
            pushMoveImageCommand(_shiftImageBeforePos,
                                 after,
                                 tr("Перемещение изображения"));
        }

        mouseEvent->accept();
    }

    if (mouseEvent->button() == Qt::LeftButton)
    {
        _isDraggingImage = false;
        _draggingItem    = nullptr;

        if (_isInDrawingMode)
        {
            _isInDrawingMode = false;
            setSceneItemsMovable(true);
        }
    }
    if (mouseEvent->button() == Qt::LeftButton)
    {
        if (_movingItem)
        {
            _movingItem->setFlags(_moveSavedFlags);

            if (_moveHadChanges)
            {
                if (_moveIsGroup && !_movingItems.isEmpty())
                {
                    // Групповое перемещение
                    QString descr = (_movingItems.size() == 1)
                                      ? tr("Перемещение фигуры")
                                      : tr("Перемещение фигур");

                    if (QUndoStack* st = activeUndoStack())
                        st->beginMacro(descr);

                    const int count = qMin(_movingItems.size(), _moveGroupBefore.size());
                    for (int i = 0; i < count; ++i)
                    {
                        QGraphicsItem* item = _movingItems[i];
                        if (!item)
                            continue;

                        const ShapeBackup& before = _moveGroupBefore[i];
                        ShapeBackup after = makeBackupFromItem(item);

                        if (!sameGeometry(before, after))
                        {
                            pushMoveShapeCommand(item, before, after, descr);
                        }
                    }

                    if (QUndoStack* st = activeUndoStack())
                        st->endMacro();
                }
                else
                {
                    // Перемещение одной фигуры
                    ShapeBackup after = makeBackupFromItem(_movingItem);

                    if (!sameGeometry(_moveBeforeSnap, after))
                    {
                        pushMoveShapeCommand(_movingItem,
                                             _moveBeforeSnap,
                                             after,
                                             tr("Перемещение фигуры"));
                    }
                }
            }

            _movingItem = nullptr;
            _movingItems.clear();
            _moveGroupBefore.clear();
            _moveGroupInitialPos.clear();
            _moveIsGroup = false;
            _moveHadChanges = false;
        }
    }



    if (mouseEvent->button() == Qt::LeftButton && _isDrawingRectangle)
    {
        _isDrawingRectangle = false;
        // Получаем финальный прямоугольник и нормализуем его (если размеры отрицательны)
        QRectF finalRect = _currRectangle->sceneBoundingRect();
        // Убираем временный прямоугольник из сцены
        _scene->removeItem(_currRectangle);
        delete _currRectangle;
        _currRectangle = nullptr; // Сбрасываем ссылку, чтобы начать новый прямоугольник

        // Проверяем минимальный размер
        if (finalRect.width() < 1 || finalRect.height() < 1)
            return; // Не создаем объект, если размеры некорректны

        // Создаем объект класса Rectangle
        qgraph::Rectangle* rectangle = new qgraph::Rectangle(_scene);
        rectangle->setRealSceneRect(finalRect); // Устанавливаем координаты и размеры
        rectangle->updatePointNumbers();
        apply_LineWidth_ToItem(rectangle);
        apply_PointSize_ToItem(rectangle);
        apply_NumberSize_ToItem(rectangle);

        // Получаем список классов из listWidget_PolygonLabel
        // QStringList classes;
        // for (int i = 0; i < ui->polygonLabel->count(); ++i)
        // {
        //     classes << ui->polygonLabel->item(i)->text();
        // }

        // // Показываем диалог выбора класса
        // SelectClass dialog(classes, this);

        SelectClass dialog(_projectClasses, this);

        if (dialog.exec() == QDialog::Accepted)
        {
            QString selectedClass = dialog.selectedClass();
            if (!selectedClass.isEmpty())
            {
                rectangle->setData(0, selectedClass);
                applyClassColorToItem(rectangle, selectedClass);
                linkSceneItemToList(rectangle); // Связываем новый элемент с списком

                ShapeBackup b = makeBackupFromItem(rectangle);
                pushAdoptExistingShapeCommand(rectangle, b, tr("Добавление прямоугольника"));

            }
        }
        if (auto doc = currentDocument())
        {
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
        _scene->removeItem(_currCircle);
        delete _currCircle;
        _currCircle = nullptr;

        // Удаляем временный крестик центра
        if (_currCircleCrossV)
        {
            _scene->removeItem(_currCircleCrossV);
            delete _currCircleCrossV;
            _currCircleCrossV = nullptr;
        }
        if (_currCircleCrossH)
        {
            _scene->removeItem(_currCircleCrossH);
            delete _currCircleCrossH;
            _currCircleCrossH = nullptr;
        }

        // Проверяем минимальный радиус
        if (circleRect.width() < 10 || circleRect.height() < 10)
            return; // Прерываем, если круг слишком маленький


        // Создаем объект класса Circle
        qgraph::Circle* circle = new qgraph::Circle(_scene, _startPoint);
        // Устанавливаем радиус финального круга
        qreal finalRadius = circleRect.width() / 2;
        circle->setRealRadius(static_cast<int>(finalRadius));
        apply_LineWidth_ToItem(circle);
        apply_PointSize_ToItem(circle);

        // QStringList classes;
        // for (int i = 0; i < ui->polygonLabel->count(); ++i)
        // {
        //     classes << ui->polygonLabel->item(i)->text();
        // }

        // SelectClass dialog(classes, this);

        SelectClass dialog(_projectClasses, this);

        if (dialog.exec() == QDialog::Accepted)
        {
            QString selectedClass = dialog.selectedClass();
            if (!selectedClass.isEmpty())
            {
                circle->setData(0, selectedClass);
                applyClassColorToItem(circle, selectedClass);
                linkSceneItemToList(circle); // Связываем новый элемент с списком

                ShapeBackup b = makeBackupFromItem(circle);
                pushAdoptExistingShapeCommand(circle, b, tr("Добавление круга"));

            }
        }
        if (auto doc = currentDocument())
        {
            doc->isModified = true;
            updateFileListDisplay(doc->filePath);
        }
        raiseAllHandlesToTop();
    }
    else if (_drawingPolyline && _polyline && _polyline->isClosed())
    {
        _drawingPolyline = false;

        _polyline->updatePointNumbers();
        apply_LineWidth_ToItem(_polyline);
        apply_PointSize_ToItem(_polyline);
        apply_NumberSize_ToItem(_polyline);

        // Получаем список классов из listWidget_PolygonLabel
        // QStringList classes;
        // for (int i = 0; i < ui->polygonLabel->count(); ++i)
        //     classes << ui->polygonLabel->item(i)->text();
        // // Показываем диалог выбора класса
        // SelectClass dialog(classes, this);

        if (!_resumeEditing)
        {
            SelectClass dialog(_projectClasses, this);
            if (dialog.exec() == QDialog::Accepted)
            {
                const QString selectedClass = dialog.selectedClass();
                if (!selectedClass.isEmpty())
                {
                    _polyline->setData(0, selectedClass);
                    applyClassColorToItem(_polyline, selectedClass);
                    linkSceneItemToList(_polyline);

                    ShapeBackup b = makeBackupFromItem(_polyline);
                    pushAdoptExistingShapeCommand(_polyline, b, tr("Добавление полилинии"));
                }
            }
        }
        _polyline = nullptr;

        if (auto doc = currentDocument())
        {
            doc->isModified = true;
            updateFileListDisplay(doc->filePath);
        }
        raiseAllHandlesToTop();
    }
    else if (_drawingLine && _line && _line->isClosed())
    {
        _drawingLine = false;

        _line->updatePointNumbers();
        apply_LineWidth_ToItem(_line);
        apply_PointSize_ToItem(_line);
        apply_NumberSize_ToItem(_line);

        // Получаем список классов из listWidget_PolygonLabel
        // QStringList classes;
        // for (int i = 0; i < ui->polygonLabel->count(); ++i)
        //     classes << ui->polygonLabel->item(i)->text();

        // // Показываем диалог выбора класса
        // SelectClass dialog(classes, this);

        if (!_resumeEditing)
        {
            SelectClass dialog(_projectClasses, this);
            if (dialog.exec() == QDialog::Accepted)
            {
                const QString selectedClass = dialog.selectedClass();
                if (!selectedClass.isEmpty())
                {
                    _line->setData(0, selectedClass);
                    applyClassColorToItem(_line, selectedClass);
                    linkSceneItemToList(_line);

                    ShapeBackup b = makeBackupFromItem(_line);
                    pushAdoptExistingShapeCommand(_line, b, tr("Добавление линии"));
                }
            }
        }
        _line = nullptr;

        if (auto doc = currentDocument())
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
        //_isInDrawingMode = false;
        //setSceneItemsMovable(true);

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


    Document::Ptr doc = currentDocument();
    if (doc && mouseEvent->button() == Qt::LeftButton)
    {
        if (_isAllMoved)
        {
            _isAllMoved = false;
            return;
        }
        // Проверяем, были ли перемещены какие-либо фигуры
        bool anyItemMoved = false;
        for (QGraphicsItem* item : _scene->items())
        {
            if (item != _videoRect &&
                !dynamic_cast<DragCircle*>(item) &&
                item->flags() & QGraphicsItem::ItemIsMovable &&
                item->pos() != QPointF(0, 0))
            {
                anyItemMoved = true;
                break;
            }
        }

        if (anyItemMoved && !doc->isModified)
        {
            doc->isModified = true;
            updateFileListDisplay(doc->filePath);
        }
    }
    //graphView->mouseReleaseEvent(mouseEvent);
}

void MainWindow::setSceneItemsMovable(bool movable)
{
    for (QGraphicsItem* item : _scene->items())
    {
        if (!item)
            continue;

        // Не трогаем дочерние элементы (ручки, контуры)
        if (item->parentItem() != nullptr)
            continue;

        if (item != _videoRect &&
            item != _tempRectItem &&
            item != _tempCircleItem &&
            item != _tempPolyline &&
            item != _currCircle &&
            item != _currCircleCrossV &&
            item != _currCircleCrossH)
        {
            item->setFlag(QGraphicsItem::ItemIsMovable, movable);
        }
    }
}

Document::Ptr MainWindow::currentDocument() const
{
    if (!ui || !ui->fileList)
    {
        return nullptr;
    }

    QListWidgetItem* currentItem = ui->fileList->currentItem();
    if (!currentItem)
    {
        return nullptr;
    }

    QVariant data = currentItem->data(Qt::UserRole);
    if (!data.isValid() || !data.canConvert<Document::Ptr>())
    {
        return nullptr;
    }

    return data.value<Document::Ptr>();
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
        case QEvent::MouseButtonPress:
        {
            auto* me = static_cast<QMouseEvent*>(event);
            if (me->button() != Qt::LeftButton)
                break;

            const QPointF sp = ui->graphView->mapToScene(me->pos());
            bool topIsHandle = false;

            if (_drawingPolyline && _polyline &&
                qgraph::Polyline::globalCloseMode() == qgraph::Polyline::CloseMode::SingleClickOnFirstPoint)
            {
                const auto& circles = _polyline->circles();
                if (!circles.isEmpty())
                {
                    const QPointF p0 = circles.first()->scenePos();
                    constexpr qreal kHit = 8.0;
                    if (QLineF(sp, p0).length() <= kHit)
                    {
                        _drawingPolyline = false;
                        _polyline->closePolyline();

                        _polyline->updatePointNumbers();
                        apply_LineWidth_ToItem(_polyline);
                        apply_PointSize_ToItem(_polyline);
                        apply_NumberSize_ToItem(_polyline);

                        // Выбор класса
                        // QStringList classes;
                        // for (int i = 0; i < ui->polygonLabel->count(); ++i)
                        //     classes << ui->polygonLabel->item(i)->text();

                        // SelectClass dialog(classes, this);

                        if (!_resumeEditing)
                        {
                            SelectClass dialog(_projectClasses, this);
                            if (dialog.exec() == QDialog::Accepted)
                            {
                                const QString selectedClass = dialog.selectedClass();
                                if (!selectedClass.isEmpty())
                                {
                                    _polyline->setData(0, selectedClass);
                                    applyClassColorToItem(_polyline, selectedClass);
                                    linkSceneItemToList(_polyline);

                                    ShapeBackup b = makeBackupFromItem(_polyline);
                                    pushAdoptExistingShapeCommand(_polyline, b, tr("Добавление полилинии"));
                                }
                            }
                        }

                        _polyline = nullptr;

                        if (auto doc = currentDocument())
                        {
                            doc->isModified = true;
                            updateFileListDisplay(doc->filePath);
                        }
                        raiseAllHandlesToTop();

                        event->accept();
                        return true;
                    }
                }
            }

            if (auto* target = pickHiddenHandle(sp, topIsHandle))
            {
                if (target->scene() && _scene && _scene->items().contains(target))
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
            auto* me = static_cast<QMouseEvent*>(event);
            const QPointF sp = ui->graphView->mapToScene(me->pos());

            if (m_isDraggingHandle && m_dragHandle)
            {
                // Проверяем валидность указателя перед обновлением
                if (m_dragHandle && m_dragHandle->scene())
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
            auto* me = static_cast<QMouseEvent*>(event);
            if (me->button() != Qt::LeftButton)
                break;

            if (m_isDraggingHandle && m_dragHandle)
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
            //auto* ke = static_cast<QKeyEvent*>(event);

            // Завершение рисования по 'C' при выбранном режиме KeyC
            if (_drawingPolyline && _polyline &&
                qgraph::Polyline::globalCloseMode() == qgraph::Polyline::CloseMode::KeyC)
            {
                event->accept();
                return true;
            }

            // Завершение рисования по 'C' при выбранном режиме KeyC для линии
            if (_drawingLine && _line &&
                qgraph::Line::globalCloseMode() == qgraph::Line::CloseMode::KeyC)
            {
                event->accept();
                return true;
            }
            break;
        }
        case QEvent::Wheel:
        {
            if (obj != ui->graphView->viewport())
                break;

            auto* we = static_cast<QWheelEvent*>(event);

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

            constexpr double kZoomStep = 1.10;
            constexpr double kZoomMin  = 0.10;
            constexpr double kZoomMax  = 8.00;

            QTransform base = ui->graphView->transform();
            if (m_zoom != 0.0)
            {
                QTransform inv; inv.scale(1.0 / m_zoom, 1.0 / m_zoom);
                base = base * inv;
            }

            // Новый абсолютный зум
            double nz = m_zoom * std::pow(kZoomStep, steps);
            if (nz < kZoomMin) nz = kZoomMin;
            if (nz > kZoomMax) nz = kZoomMax;

            m_zoom = nz;
            ui->graphView->setTransform(base);
            ui->graphView->scale(m_zoom, m_zoom);

            // Сохраняем состояние при изменении масштаба
            if (auto doc = currentDocument())
                saveCurrentViewState(doc);

            we->accept();
            return true;
        }
        case QEvent::ContextMenu:
        {
            auto* ce = static_cast<QContextMenuEvent*>(event);

            const QPoint  viewPos = ce->pos();
            const QPointF scenePos = ui->graphView->mapToScene(viewPos);
            QGraphicsItem* item = ui->graphView->itemAt(viewPos);

            if (auto* circle = dynamic_cast<DragCircle*>(item))
            {
                if (auto* parent = circle->parentItem())
                {
                    if (auto* polyline = dynamic_cast<qgraph::Polyline*>(parent))
                    {
                        QMenu menu(ui->graphView);
                        QAction* actRemove = menu.addAction(tr("Удалить узел"));
                        QAction* actResume = menu.addAction(tr("Возобновить редактирование"));

                        QAction* chosen = menu.exec(ce->globalPos());

                        if (chosen == actRemove)
                        {
                            ShapeBackup before = makeBackupFromItem(polyline);
                            qulonglong uid = before.uid;

                            polyline->handlePointDeletion(circle);

                            ShapeBackup after = makeBackupFromItem(polyline);
                            if (!sameGeometry(before, after))
                                pushModifyShapeCommand(uid, before, after, tr("Удаление узла"));

                            if (auto doc = currentDocument(); doc && !doc->isModified)
                            {
                                doc->isModified = true;
                                updateFileListDisplay(doc->filePath);
                            }
                        }
                        else if (chosen == actResume)
                        {
                            _resumeBefore = makeBackupFromItem(polyline);
                            _resumeUid = _resumeBefore.uid;
                            _resumeEditing = true;
                            polyline->resumeFromHandle(circle);

                            _drawingPolyline = true;
                            _polyline = polyline;
                            setSceneItemsMovable(false);

                            _polyline->setModificationCallback([this]()
                            {
                                if (_drawingPolyline && _polyline && _polyline->isClosed())
                                {
                                    _drawingPolyline = false;

                                    _polyline->updatePointNumbers();
                                    apply_LineWidth_ToItem(_polyline);
                                    apply_PointSize_ToItem(_polyline);
                                    apply_NumberSize_ToItem(_polyline);

                                    ShapeBackup after = makeBackupFromItem(_polyline);
                                    if (!sameGeometry(_resumeBefore, after))
                                        pushModifyShapeCommand(_resumeUid, _resumeBefore, after, tr("Продолжение полилинии"));

                                    _polyline = nullptr;
                                    _resumeEditing = false;
                                    _resumeUid = 0;

                                    if (auto doc = currentDocument(); doc && !doc->isModified)
                                    {
                                        doc->isModified = true;
                                        updateFileListDisplay(doc->filePath);
                                    }

                                    raiseAllHandlesToTop();
                                    setSceneItemsMovable(true);
                                }
                            });
                        }
                        ce->accept();
                        return true;
                    }
                    if (auto* line = dynamic_cast<qgraph::Line*>(parent))
                    {
                        QMenu menu(ui->graphView);

                        QAction* actRemove = menu.addAction(tr("Удалить узел"));
                        QAction* actResume = menu.addAction(tr("Возобновить редактирование"));

                        QAction* chosen = menu.exec(ce->globalPos());

                        if (chosen == actRemove)
                        {
                            ShapeBackup before = makeBackupFromItem(line);
                            qulonglong uid = before.uid;

                            line->handlePointDeletion(circle);

                            ShapeBackup after = makeBackupFromItem(line);
                            if (!sameGeometry(before, after))
                                pushModifyShapeCommand(uid, before, after, tr("Удаление узла"));

                            if (auto doc = currentDocument(); doc && !doc->isModified)
                            {
                                doc->isModified = true;
                                updateFileListDisplay(doc->filePath);
                            }
                        }
                        else if (chosen == actResume)
                        {
                            _resumeBefore = makeBackupFromItem(line);
                            _resumeUid = _resumeBefore.uid;
                            _resumeEditing = true;

                            line->resumeFromHandle(circle);

                            _drawingLine = true;
                            _line = line;

                            setSceneItemsMovable(false);

                            _line->setModificationCallback([this]()
                            {
                                if (_drawingLine && _line && _line->isClosed())
                                {
                                    _drawingLine = false;

                                    _line->updatePointNumbers();
                                    apply_LineWidth_ToItem(_line);
                                    apply_PointSize_ToItem(_line);
                                    apply_NumberSize_ToItem(_line);

                                    ShapeBackup after = makeBackupFromItem(_line);
                                    if (!sameGeometry(_resumeBefore, after))
                                        pushModifyShapeCommand(_resumeUid, _resumeBefore, after, tr("Продолжение линии"));

                                    _line = nullptr;
                                    _resumeEditing = false;
                                    _resumeUid = 0;

                                    if (auto doc = currentDocument(); doc && !doc->isModified)
                                    {
                                        doc->isModified = true;
                                        updateFileListDisplay(doc->filePath);
                                    }

                                    raiseAllHandlesToTop();
                                    setSceneItemsMovable(true);
                                }
                            });
                        }
                        ce->accept();
                        return true;
                    }

                }
            }

            if (auto* polyline = dynamic_cast<qgraph::Polyline*>(item))
            {
                const QPainterPath p = polyline->path();

                QPainterPathStroker stroker;
                const qreal w = qMax<qreal>(18.0, polyline->pen().widthF() + 14.0);
                stroker.setWidth(w);

                const QPainterPath stroke = stroker.createStroke(p);
                const QPointF localPos = polyline->mapFromScene(scenePos);
                if (!stroke.contains(localPos))
                    break;

                QMenu menu(ui->graphView);
                QAction* actAdd = menu.addAction(tr("Добавить узел"));

                QAction* chosen = menu.exec(ce->globalPos());
                if (chosen == actAdd)
                {
                    ShapeBackup before = makeBackupFromItem(polyline);
                    qulonglong uid = before.uid;

                    polyline->insertPoint(scenePos);

                    ShapeBackup after = makeBackupFromItem(polyline);
                    if (!sameGeometry(before, after))
                        pushModifyShapeCommand(uid, before, after, tr("Добавление узла"));

                    if (auto doc = currentDocument(); doc && !doc->isModified)
                    {
                        doc->isModified = true;
                        updateFileListDisplay(doc->filePath);
                    }
                }

                ce->accept();
                return true;
            }
            if (auto* line = dynamic_cast<qgraph::Line*>(item))
            {
                const QPainterPath p = line->path();

                QPainterPathStroker stroker;
                const qreal w = qMax<qreal>(18.0, line->pen().widthF() + 14.0);
                stroker.setWidth(w);

                const QPainterPath stroke = stroker.createStroke(p);
                const QPointF localPos = line->mapFromScene(scenePos);
                if (!stroke.contains(localPos))
                    break;

                QMenu menu(ui->graphView);
                QAction* actAdd = menu.addAction(tr("Добавить узел"));

                QAction* chosen = menu.exec(ce->globalPos());
                if (chosen == actAdd)
                {
                    ShapeBackup before = makeBackupFromItem(line);
                    qulonglong uid = before.uid;

                    line->insertPoint(scenePos);

                    ShapeBackup after = makeBackupFromItem(line);
                    if (!sameGeometry(before, after))
                        pushModifyShapeCommand(uid, before, after, tr("Добавление узла"));

                    if (auto doc = currentDocument(); doc && !doc->isModified)
                    {
                        doc->isModified = true;
                        updateFileListDisplay(doc->filePath);
                    }
                }

                ce->accept();
                return true;
            }
            break;
        }
        default:
            break; // Другие события сцены не интересуют
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier)
    {
        QTransform base = ui->graphView->transform();
        if (m_zoom != 0.0)
        {
            QTransform inv; inv.scale(1.0 / m_zoom, 1.0 / m_zoom);
            base = base * inv;
        }

        constexpr double kZoomStep = 1.10;
        constexpr double kZoomMin  = 0.10;
        constexpr double kZoomMax  = 8.00;

        const int key = event->key();

        if (key == Qt::Key_Plus || key == Qt::Key_Equal)
        {
            double nz = m_zoom * kZoomStep;
            if (nz < kZoomMin) nz = kZoomMin;
            if (nz > kZoomMax) nz = kZoomMax;

            m_zoom = nz;
            ui->graphView->setTransform(base);    // Вернуть базу
            ui->graphView->scale(m_zoom, m_zoom); // Применить абсолютный масштаб
            event->accept();
            return;
        }
        else if (key == Qt::Key_Minus)
        {
            double nz = m_zoom / kZoomStep;
            if (nz < kZoomMin) nz = kZoomMin;
            if (nz > kZoomMax) nz = kZoomMax;

            m_zoom = nz;
            ui->graphView->setTransform(base);
            ui->graphView->scale(m_zoom, m_zoom);
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
        QList<QGraphicsItem*> selectedItems = _scene->selectedItems();
        for (QGraphicsItem* item : selectedItems)
        {
            // Если выделена ручка, удаляем ее владельца (фигуру), а не саму ручку
            if (auto* h = dynamic_cast<qgraph::DragCircle*>(item))
            {
                if (h->parentItem())
                    item = h->parentItem();
            }

            // Сначала убираемм связанный пункт из списка фигур
            onSceneItemRemoved(item);

            // Затем удалим со сцены и из памяти

            // _scene->removeItem(item);
            // delete item;
            if (item)
            {
                if (auto sc = item->scene())
                    sc->removeItem(item);
                delete item;
            }
        }
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
        event->accept();
        return;
    }
    else if (event->key() == Qt::Key_B)
    {
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

void MainWindow::togglePointerMode()
{
//    ui->graphView->setDragMode(_selectModeButton->isChecked()
//                              ? QGraphicsView::RubberBandDrag
//                              : QGraphicsView::ScrollHandDrag);
//    // Это свойство определяет, позволяет ли представление
//    // взаимодействовать со сценой.
//    ui->graphView->setInteractive(_selectModeButton->isChecked());
}

void MainWindow::on_actOpen_triggered(bool)
{
    QString initialDir = _lastUsedFolder.isEmpty()
                             ? QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
                             : _lastUsedFolder;

    // Получаем домашнюю директорию пользователя как начальную для диалога
    //QString initialDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);

    // Создаем диалог выбора директории
    QString folderPath = QFileDialog::getExistingDirectory(
        this,
        tr("Выберите рабочую папку с изображениями"),
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
    Document::Ptr doc = currentDocument();
    if (!doc)
    {
        return;
    }
    saveAnnotationToFile(doc);
}

void MainWindow::on_actExit_triggered(bool)
{
    //qApp->quit();
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

void MainWindow::on_btnTest_clicked(bool)
{

}

void MainWindow::closeEvent(QCloseEvent* event)
{
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

void MainWindow::loadFilesFromFolder(const QString& folderPath)
{
    QDir directory(folderPath);
    _currentFolderPath = folderPath; // Сохраняем путь

    // Путь к папке в заголовке окна
    updateWindowTitle();
    //Путь к папке внизу окна
    updateFolderPathDisplay();

    ui->fileList->clear(); // Очищаем список перед добавлением
    // Путь к папке над полем FileList
    ui->listPath->setText(QDir::toNativeSeparators(folderPath));


    QStringList filters;
    filters << "*.jpg";
    filters << "*.jpeg";
    filters << "*.png";
    QStringList files = directory.entryList(filters, QDir::Files);

    // Загружаем иконки из ресурсов
    QIcon modifiedIcon(":/images/resources/ok.svg");
    QIcon defaultIcon(":/images/resources/not_ok.svg");

    // Добавляем файлы
    for (auto filename : files)
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

        QUndoStack* st = doc->_undoStack.get();
        QObject::connect(st, &QUndoStack::indexChanged, this, [this, st](int){
            if (_loadingNow) return;

            // реагируем только если этот стек активен у группы
            if (_undoGroup && _undoGroup->activeStack() != st) return;

            if (auto d = currentDocument())
            {
                // Если стек в состоянии <пусто>, считаем файл неизмененным
                const bool clean = st->isClean();
                d->isModified = !clean;
                updateFileListDisplay(d->filePath);
            }
        });

        // Убираем чекбоксы
        //item->setFlags(item->flags() & ~Qt::ItemIsUserCheckable);

        ui->fileList->addItem(item);
    }
}

void MainWindow::updatePolygonListForCurrentScene()
{
    ui->polygonList->clear();

    if (ui->coordinateList)
            ui->coordinateList->clear();

    if (!_scene)
        return;

    for (QGraphicsItem* item : _scene->items())
    {
        if (item != _videoRect &&
            item != _tempRectItem &&
            item != _tempCircleItem &&
            item != _tempPolyline)
        {
            QString className = item->data(0).toString();
            if (!className.isEmpty())
            {
                linkSceneItemToList(item);
            }
        }
    }
}

void MainWindow::loadGeometry()
{
    // QVector<int> v {100, 100, 800, 600};
    // config::base().getValue("windows.main_window.geometry", v);
    // setGeometry(v[0], v[1], v[2], v[3]);

    QVector<int> wg;
    config::base().getValue("windows.main_window.geometry", wg);
    if (wg.count() != 4)
    {
        //wg.resize(4);
        wg = {10, 10, 1400, 800};
        //setGeometry(wg[0], wg[1], wg[2], wg[3]);

        QList<QScreen*> screens = QGuiApplication::screens();
        if (!screens.isEmpty())
        {
            QRect screenGeometry = screens[0]->geometry();
            //QRect windowGeometry = this->geometry();
            QRect windowGeometry {0, 0, int(screenGeometry.width()  * (2./3)),
                                        int(screenGeometry.height() * (2./3))};
            wg[0] = screenGeometry.width()  / 2 - windowGeometry.width()  / 2;
            wg[1] = screenGeometry.height() / 2 - windowGeometry.height() / 2;
            wg[2] = windowGeometry.width();
            wg[3] = windowGeometry.height();
        }
    }
    setGeometry(wg[0], wg[1], wg[2], wg[3]);

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

//    int tabIndex = 0;
//    config::base().getValue("windows.main_window.tab_index", tabIndex);
//    ui->tabWidget->setCurrentIndex(tabIndex);

//    QString sval;
//    config::base().getValue("windows.main_window.event_journal_header", sval);
//    if (!sval.isEmpty())
//    {
//        QByteArray ba = QByteArray::fromBase64(sval.toLatin1());
//        ui->tableJournal->horizontalHeader()->restoreState(ba);
//    }
}

void MainWindow::saveGeometry()
{
    QRect g = geometry();
    QVector<int> v {g.x(), g.y(), g.width(), g.height()};
    config::base().setValue("windows.main_window.geometry", v);

    QList<int> splitterSizes = ui->splitter->sizes();
    config::base().setValue("windows.main_window.splitter_sizes", splitterSizes);

    splitterSizes = ui->splitter2->sizes();
    config::base().setValue("windows.main_window.splitter2_sizes", splitterSizes);

//    config::base().setValue("windows.main_window.tab_index", ui->tabWidget->currentIndex());

//    QByteArray ba = ui->tableJournal->horizontalHeader()->saveState().toBase64();
//    config::base().setValue("windows.main_window.event_journal_header", QString::fromLatin1(ba));
    saveVisualStyle();
}

QString MainWindow::annotationPathFor(const QString& imagePath)
{
    QFileInfo fileInfo(imagePath);
    QDir dir(fileInfo.absolutePath());
    return dir.filePath(fileInfo.completeBaseName() + ".yaml");
}

void MainWindow::saveAnnotationToFile(Document::Ptr doc)
{
    if (!doc || !doc->scene || !doc->videoRect)
    {
        return;
    }

    YamlConfig::Func saveCircles = [&](YamlConfig* conf, YAML::Node& ycircles, bool)
    {
        for (QGraphicsItem* item : doc->scene->items())
            if (qgraph::Circle* circle = dynamic_cast<qgraph::Circle*>(item))
            {
                YAML::Node ycircle;

                QString className = circle->data(0).toString();
                if (className.isEmpty())
                    className = "none";

                conf->setValue(ycircle, "label",  className);

                QPoint center = circle->center();
                int radius = circle->realRadius();

                // Математическое округление координат центра
                int centerX = std::round(center.x());
                int centerY = std::round(center.y());
                QPoint roundedCenter(centerX, centerY);

                conf->setValue(ycircle, "center", roundedCenter   );
                conf->setValue(ycircle, "radius", radius   );

                ycircles.push_back(ycircle);
            }
        return true;
    };

    YamlConfig::Func savePolylines = [&](YamlConfig* conf, YAML::Node& ypolylines, bool)
    {
        for (QGraphicsItem* item : doc->scene->items())
            if (qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(item))
            {
                YAML::Node ypolyline;

                QString className = polyline->data(0).toString();
                if (className.isEmpty())
                    className = "none";

                conf->setValue(ypolyline, "label", className);

                QVector<QPointF> points = polyline->points();
                YamlConfig::Func savePoints = [&points](YamlConfig* conf, YAML::Node& ypoints, bool)
                {
                    for (const QPointF& point : points)
                    {
                        YAML::Node ypoint;
                        // Математическое округление координат точек
                        int x = std::round(point.x());
                        int y = std::round(point.y());
                        conf->setValue(ypoint, "x", x);
                        conf->setValue(ypoint, "y", y);
                        ypoints.push_back(ypoint);
                    }
                    return true;
                };
                conf->setValue(ypolyline, "points", savePoints);
                conf->setNodeStyle(ypolyline, "points", YAML::EmitterStyle::Flow);

                ypolylines.push_back(ypolyline);
            }

        return true;
    };
    YamlConfig::Func saveRectangles = [&](YamlConfig* conf, YAML::Node& yrectangles, bool)
    {
        for (QGraphicsItem* item : doc->scene->items())
            if (qgraph::Rectangle* rect= dynamic_cast<qgraph::Rectangle*>(item))
            {
                YAML::Node yrectangle;

                QString className = rect->data(0).toString();
                if (className.isEmpty())
                    className = "none";

                conf->setValue(yrectangle, "label", className);

                QRectF sceneRect = rect->sceneBoundingRect();

                // Математическое округление координат углов
                QPoint topLeft(std::round(sceneRect.topLeft().x()),
                               std::round(sceneRect.topLeft().y()));
                QPoint bottomRight(std::round(sceneRect.bottomRight().x()),
                                   std::round(sceneRect.bottomRight().y()));

                conf->setValue(yrectangle, "point1", topLeft);
                conf->setValue(yrectangle, "point2", bottomRight);
                yrectangles.push_back(yrectangle);
            }

        return true;
    };
    YamlConfig::Func savePoints = [&](YamlConfig* conf, YAML::Node& ypoints, bool)
    {
        for (QGraphicsItem* item : doc->scene->items())
            if (qgraph::Point* p = dynamic_cast<qgraph::Point*>(item))
            {
                YAML::Node ypoint;

                QString className = p->data(0).toString();
                if (className.isEmpty())
                    className = "none";

                conf->setValue(ypoint, "label", className);

                QPoint center = p->center();
                conf->setValue(ypoint, "point", center);

                ypoints.push_back(ypoint);
            }
        return true;
    };
    YamlConfig::Func saveLines = [&](YamlConfig* conf, YAML::Node& ylines, bool)
    {
        for (QGraphicsItem* item : doc->scene->items())
            if (qgraph::Line* line = dynamic_cast<qgraph::Line*>(item))
            {
                YAML::Node yline;

                QString className = line->data(0).toString();
                if (className.isEmpty())
                    className = "none";

                conf->setValue(yline, "label", className);

                QVector<QPointF> points = line->points();
                YamlConfig::Func savePoints = [&points](YamlConfig* conf, YAML::Node& ypoints, bool)
                {
                    for (const QPointF& point : points)
                    {
                        YAML::Node ypoint;
                        // Математическое округление координат точек
                        int x = std::round(point.x());
                        int y = std::round(point.y());
                        conf->setValue(ypoint, "x", x);
                        conf->setValue(ypoint, "y", y);
                        ypoints.push_back(ypoint);
                    }
                    return true;
                };
                conf->setValue(yline, "points", savePoints);
                conf->setNodeStyle(yline, "points", YAML::EmitterStyle::Flow);

                ylines.push_back(yline);
            }
        return true;
    };

    YamlConfig::Func saveFunc = [&](YamlConfig* conf, YAML::Node& shapes, bool /*logWarn*/)
    {

        conf->setValue(shapes, "circles", saveCircles);
        conf->setValue(shapes, "polylines", savePolylines);
        conf->setValue(shapes, "rectangles", saveRectangles);
        conf->setValue(shapes, "points", savePoints);
        conf->setValue(shapes, "lines", saveLines);

        return true;
    };

    YamlConfig yconfig;
    yconfig.setValue("shapes", saveFunc);
    //yconfig.saveFile("/tmp/1.yaml");
    //yconfig.saveFile("/home/marie/тестовые данные QLabelMe/фото/1.yaml");

    // QFileInfo fileInfo(doc->filePath);
    // QString yamlPath = fileInfo.path() + "/" + fileInfo.completeBaseName() + ".yaml";
    // yconfig.saveFile(yamlPath.toStdString());

    const QString yamlPath = annotationPathFor(doc->filePath);
    // Для внешних C/STD API на Windows используем нативную кодировку ОС
    const QByteArray encoded = QFile::encodeName(QDir::toNativeSeparators(yamlPath));
    yconfig.saveFile(std::string(encoded.constData(), encoded.size()));

    // После успешного сохранения обновляем отображение в списке файлов
    if (QFile::exists(yamlPath))
    {
        // Находим соответствующий элемент в списке и убираем звездочку
        for (int i = 0; i < ui->fileList->count(); ++i)
        {
            QListWidgetItem* item = ui->fileList->item(i);
            QVariant data = item->data(Qt::UserRole);
            if (data.canConvert<Document::Ptr>())
            {
                Document::Ptr itemDoc = data.value<Document::Ptr>();
                if (itemDoc->filePath == doc->filePath)
                {
                    // Убираем звездочку из названия
                    QString fileName = QFileInfo(doc->filePath).fileName();
                    item->setText(fileName);
                    break;
                }
            }
        }
        QListWidgetItem* item = findFileListItem(doc->filePath);
        if (item)
        {
            updateFileListItemIcon(item, true);
        }
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

void MainWindow::loadAnnotationFromFile(Document::Ptr doc)
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

    // Получаем путь к файлу аннотаций (заменяем расширение изображения на .yaml)
    // QFileInfo fileInfo(doc->filePath);
    // QString yamlPath = fileInfo.path() + "/" + fileInfo.completeBaseName() + ".yaml";

    // YamlConfig yconfig;
    // if (!yconfig.readFile(yamlPath.toStdString()))
    // {
    //     log_error_m << "Ошибка при загрузке разметки:" << yamlPath;
    //     _loadingNow = false;
    //     return;
    // }

    const QString yamlPath = annotationPathFor(doc->filePath);
    YamlConfig yconfig;

    const QByteArray encoded = QFile::encodeName(QDir::toNativeSeparators(yamlPath));
    if (!yconfig.readFile(std::string(encoded.constData(), encoded.size())))
    {
        log_error_m << "Ошибка при загрузке разметки:" << yamlPath;
        _loadingNow = false;
        return;
    }

    //if (yconfig.readFile("/tmp/1.yaml"))
    //if (!yconfig.readFile("/home/marie/тестовые данные QLabelMe/фото/1.yaml"))


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

    YamlConfig::Func loadCircles = [&](YamlConfig* conf, YAML::Node& ycircles, bool)
    {
        for (const YAML::Node& ycircle : ycircles)
        {
            QString label;
            conf->getValue(ycircle, "label", label);

            QPoint center;
            conf->getValue(ycircle, "center", center);

            int radius = 0;
            conf->getValue(ycircle, "radius", radius);

            qgraph::Circle* circle = new qgraph::Circle(doc->scene, QPointF(center) + imageOffset);
            circle->setRealRadius(radius);
            circle->setData(0, label);
            apply_LineWidth_ToItem(circle);
            apply_PointSize_ToItem(circle);
            applyClassColorToItem(circle, label);
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
                    int x = 0, y = 0;
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
            for (int i = 1; i < points.size(); ++i)
            {
                polyline->addPoint(points[i], doc->scene);
            }
            polyline->closePolyline();
            polyline->updatePath();
            polyline->setData(0, label);
            apply_LineWidth_ToItem(polyline);
            apply_PointSize_ToItem(polyline);
            apply_NumberSize_ToItem(polyline);
            applyClassColorToItem(polyline, label);
        }
        return true;
    };
    YamlConfig::Func loadRectangles = [&](YamlConfig* conf, YAML::Node& yrectangles, bool)
    {
        for (const YAML::Node& yrectangle : yrectangles)
        {
            QString label;
            conf->getValue(yrectangle, "label", label);

            QPoint point1, point2;
            conf->getValue(yrectangle, "point1", point1);
            conf->getValue(yrectangle, "point2", point2);

            // Создаем прямоугольник на сцене
            qgraph::Rectangle* rect = new qgraph::Rectangle(doc->scene);
            rect->setRealSceneRect(QRectF(QPointF(point1) + imageOffset, QPointF(point2) + imageOffset));
            rect->setData(0, label);
            apply_LineWidth_ToItem(rect);
            apply_PointSize_ToItem(rect);
            apply_NumberSize_ToItem(rect);
            applyClassColorToItem(rect, label);
        }

        return true;
    };
    YamlConfig::Func loadPoints = [&](YamlConfig* conf, YAML::Node& ypoints, bool)
    {
        for (const YAML::Node& ypoint : ypoints)
        {
            QString label;
            conf->getValue(ypoint, "label", label);

            QPoint center;
            conf->getValue(ypoint, "point", center);

            qgraph::Point* p = new qgraph::Point(doc->scene);
            p->setCenter(center + imageOffset);
            p->setData(0, label);

            apply_PointSize_ToItem(p);
            apply_NumberSize_ToItem(p);
            apply_PointStyle_ToItem(p);
            applyClassColorToItem(p, label);
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
                    int x = 0, y = 0;
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
            for (int i = 1; i < points.size(); ++i)
            {
                line->addPoint(points[i], doc->scene);
            }
            line->closeLine();
            line->updatePath();
            line->setData(0, label);
            apply_LineWidth_ToItem(line);
            apply_PointSize_ToItem(line);
            apply_NumberSize_ToItem(line);
            applyClassColorToItem(line, label);
        }
        return true;
    };

    YamlConfig::Func loadFunc = [&](YamlConfig* conf, YAML::Node& shapes, bool /*logWarn*/)
    {

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
    // После загрузки всех элементов обновляем список
    updatePolygonListForCurrentScene();
}

void MainWindow::deserializeJsonToScene(QGraphicsScene* scene, const QJsonObject& json)
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

            qgraph::Rectangle* rect = new qgraph::Rectangle(scene);
            //rect->setRect(QRectF(x, y, width, height));
            rect->setRealSceneRect(QRectF(QPointF(x, y), QSizeF(width, height)));
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

            qgraph::Circle* circle = new qgraph::Circle(scene, QPointF(x, y));
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

            QPointF firstPoint(pointsArray[0].toObject()["x"].toDouble(),
                               pointsArray[0].toObject()["y"].toDouble());

            qgraph::Polyline* polyline = new qgraph::Polyline(scene, firstPoint);
            for (int i = 1; i < pointsArray.size(); ++i)
            {
                QPointF point(pointsArray[i].toObject()["x"].toDouble(),
                              pointsArray[i].toObject()["y"].toDouble());
                polyline->addPoint(point, scene);
            }
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

            qgraph::Line* line = new qgraph::Line(scene, firstPoint);

            for (int i = 1; i < pointsArray.size(); ++i)
            {
                QJsonObject ptObj = pointsArray[i].toObject();
                QPointF p(ptObj["x"].toDouble(), ptObj["y"].toDouble());
                line->addPoint(p, scene);
            }
            line->setData(0, className);

            apply_LineWidth_ToItem(line);
            apply_PointSize_ToItem(line);
            apply_NumberSize_ToItem(line);
        }
        else if (type == "point")
        {
            qreal x = shapeObj["x"].toDouble();
            qreal y = shapeObj["y"].toDouble();

            qgraph::Point* p = new qgraph::Point(scene);
            p->setCenter(QPointF(x, y));
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

qgraph::VideoRect* MainWindow::findVideoRect(QGraphicsScene* scene)
{
    if (!scene)
    {
        return nullptr;
    }

    foreach(QGraphicsItem* item, scene->items())
    {
        if (auto* videoRect = dynamic_cast<qgraph::VideoRect*>(item))
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

void MainWindow::updateFolderPathDisplay()
{
    // if (_currentFolderPath.isEmpty())
    // {
    //     _folderPathLabel->clear();
    //     _folderPathLabel->setToolTip("");
    // }
    // else
    // {
    //     QString displayPath = QDir::toNativeSeparators(_currentFolderPath);
    //     _folderPathLabel->setText(displayPath);
    //     _folderPathLabel->setToolTip(displayPath); // Полный путь в подсказке

    //     // Можно добавить иконку папки
    //     //QIcon folderIcon = style()->standardIcon(QStyle::SP_DirIcon);
    //     //_folderPathLabel->setPixmap(folderIcon.pixmap(16, 16));
    // }
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
            Circle c;
            c.label  = label;
            c.x  = o["x"].toDouble();
            c.y = o["y"].toDouble();
            c.radius = o["radius"].toDouble();
            circles.append(c);
        }
        else if (type == "rectangle")
        {
            Rectangle r;
            r.label = label;
            double x = o["x"].toDouble();
            double y = o["y"].toDouble();
            double w = o["width"].toDouble();
            double h = o["height"].toDouble();
            r.x1 = x;
            r.y1 = y;
            r.x2 = x + w;
            r.y2 = y + h;
            rectangles.append(r);
        }
        else if (type == "polyline")
        {
            Polyline pl;
            pl.label = label;
            QJsonArray pts = o["points"].toArray();
            for (const QJsonValue& pv : pts)
            {
                QJsonObject po = pv.toObject();
                pl.points.append(QPointF(po["x"].toDouble(),
                                         po["y"].toDouble()));
            }
            polylines.append(pl);
        }
        else if (type == "line")
        {
            Line ln;
            ln.label = label;
            QJsonArray pts = o["points"].toArray();
            for (const QJsonValue& pv : pts)
            {
                QJsonObject po = pv.toObject();
                ln.points.append(QPointF(po["x"].toDouble(),
                                         po["y"].toDouble()));
            }
            lines.append(ln);
        }
        else if (type == "point")
        {
            Point p;
            p.label = label;
            p.x     = o["x"].toDouble();
            p.y     = o["y"].toDouble();
            points.append(p);
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
            for (const Polyline& pl : polylines)
            {
                out << "    - label: " << pl.label << "\n";
                out << "      points: [";
                for (int i = 0; i < pl.points.size(); ++i)
                {
                    const QPointF& p = pl.points[i];
                    int px = static_cast<int>(std::round(p.x()));
                    int py = static_cast<int>(std::round(p.y()));
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

    // Пишем и JSON, и YAML в буфер обмена
    auto* mime = new QMimeData;
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


    if (!scene)
    {
        root["shapes"] = shapesArray;
        return root;
    }

    for (QGraphicsItem* item : scene->items())
    {
        // Пропускаем само изображение и временные элементы
        if (item == _videoRect || item == _tempRectItem ||
            item == _tempCircleItem || item == _tempPolyline)
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

        if (auto* rect = dynamic_cast<qgraph::Rectangle*>(item))
        {
            shapeObj["type"] = "rectangle";
            // QRectF r = rect->rect();
            QRectF r = rect->sceneBoundingRect();
            shapeObj["x"] = r.x();
            shapeObj["y"] = r.y();
            shapeObj["width"] = r.width();
            shapeObj["height"] = r.height();
        }
        else if (auto* circle = dynamic_cast<qgraph::Circle*>(item))
        {
            shapeObj["type"] = "circle";
            //QPointF center = circle->realCenter();
            QPointF center = circle->center();
            shapeObj["x"] = center.x();
            shapeObj["y"] = center.y();
            shapeObj["radius"] = circle->realRadius();
        }
        else if (auto* polyline = dynamic_cast<qgraph::Polyline*>(item))
        {
            shapeObj["type"] = "polyline";
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
        else if (auto* line = dynamic_cast<qgraph::Line*>(item))
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
        else if (auto* point = dynamic_cast<qgraph::Point*>(item))
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

QJsonObject MainWindow::serializeSelectedItemsToJson(QGraphicsScene* scene)
{
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
        if (item == _videoRect || item == _tempRectItem ||
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

        if (auto* rect = dynamic_cast<qgraph::Rectangle*>(item))
        {
            shapeObj["type"] = "rectangle";
            //QRectF r = rect->rect();
            QRectF r = rect->sceneBoundingRect();
            shapeObj["x"] = r.x();
            shapeObj["y"] = r.y();
            shapeObj["width"] = r.width();
            shapeObj["height"] = r.height();
        }
        else if (auto* circle = dynamic_cast<qgraph::Circle*>(item))
        {
            shapeObj["type"] = "circle";
            //QPointF center = circle->realCenter();
            QPointF center = circle->center();
            shapeObj["x"] = center.x();
            shapeObj["y"] = center.y();
            shapeObj["radius"] = circle->realRadius();
        }
        else if (auto* polyline = dynamic_cast<qgraph::Polyline*>(item))
        {
            shapeObj["type"] = "polyline";
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
        else if (auto* line = dynamic_cast<qgraph::Line*>(item))
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
        else if (auto* point = dynamic_cast<qgraph::Point*>(item))
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
    auto doc = currentDocument();
    if (!doc || !doc->scene)
        return;

    // Собираем JSON только по выделенным фигурам на текущей сцене
    QJsonObject json = serializeSelectedItemsToJson(doc->scene);

    // Локальный буфер
    _shapesClipboard = json;

    // Отправляем JSON в системный буфер обмена
    writeShapesJsonToClipboard(json);
}

void MainWindow::pasteCopiedShapesToCurrentScene()
{
    Document::Ptr doc = currentDocument();
    if (!doc || !doc->scene)
        return;

    // Пытаемся прочитать JSON из системного буфера обмена
    QJsonObject json = readShapesJsonFromClipboard();

    // Если в системном буфере нет наших данных - используем старый локальный буфер
    if (json.isEmpty())
        json = _shapesClipboard;

    if (json.isEmpty())
        return;

    QGraphicsScene* scene = doc->scene;

    // Запоминаем, что было на сцене ДО вставки
    QList<QGraphicsItem*> beforeItems = scene->items();

    deserializeJsonToScene(scene, _shapesClipboard);
    updatePolygonListForCurrentScene();

    // Собираем список новых фигур (тех, которых не было в beforeItems)
    QList<QGraphicsItem*> afterItems = scene->items();
    QList<QGraphicsItem*> newItems;

    for (QGraphicsItem* item : afterItems)
    {
        // Пропускаем служебные элементы
        if (item == _videoRect ||
            item == _tempRectItem ||
            item == _tempCircleItem ||
            item == _tempPolyline)
        {
            continue;
        }

        // Нас интересуют только элементы, которых не было "до"
        if (beforeItems.contains(item))
            continue;

        // И только наши примитивы
        if (dynamic_cast<qgraph::Rectangle*>(item) ||
            dynamic_cast<qgraph::Circle*>(item)    ||
            dynamic_cast<qgraph::Polyline*>(item)  ||
            dynamic_cast<qgraph::Line*>(item)      ||
            dynamic_cast<qgraph::Point*>(item))
        {
            newItems.append(item);
        }
    }

    if (newItems.isEmpty())
        return;

    QString descr;
    if (newItems.size() == 1)
        descr = tr("Вставка примитива");
    else
        descr = tr("Вставка примитивов");

    if (QUndoStack* st = activeUndoStack())
        st->beginMacro(descr);

    for (QGraphicsItem* item : newItems)
    {
        ShapeBackup backup = makeBackupFromItem(item);
        pushAdoptExistingShapeCommand(item, backup, descr);
    }

    if (QUndoStack* st = activeUndoStack())
        st->endMacro();

    doc->isModified = true;
    updateFileListDisplay(doc->filePath);
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
    if (!doc || !ui->graphView || !_scene || !_videoRect)
    {
        fitImageToView();
        return;
    }

    _scene->setSceneRect(_videoRect->boundingRect().translated(_videoRect->pos()));

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
    // QFileInfo fileInfo(imagePath);
    // QString yamlPath = fileInfo.path() + "/" + fileInfo.completeBaseName() + ".yaml";
    // return QFile::exists(yamlPath);
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
    //config::dirExpansion(configFile);
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
            QMessageBox::warning(this, tr("Ошибка"),
                                 tr("Ошибка при чтении базового файла с классами!"));
            //alog::stop();
            return false;
        }
        if (!yconfig.saveFile(std::string(encoded.constData(), encoded.size())))
        {
            QMessageBox::warning(this, tr("Ошибка"),
                                 tr("Ошибка при сохранении в базовый файл с классами! "
                                    "Более подробную информацию смотри в .log файле"));
            //alog::stop();
            return false;
        }
    }
    else
    {
        if (!yconfig.readFile(std::string(encoded.constData(), encoded.size())))
        {
            QMessageBox::warning(this, tr("Ошибка"),
                                 tr("Ошибка при чтении файла с классами!"
                                    "Более подробную информацию смотри в .log файле"));
            return false;
        }
    }

    QList<QString> classes;

    if (!yconfig.getValue("classes", classes, false))
    {
        QMessageBox::warning(this, tr("Ошибка"),
                             tr("Нет секции классов"));
        return false;
    }
    QMap<QString, QColor> colors;
    QList<QString> colorLines;

    if (yconfig.getValue("class_colors", colorLines, true))
    {
        for (const QString& s : colorLines)
        {
            // ожидаем "name=#AARRGGBB"
            const int eq = s.indexOf('=');
            if (eq <= 0) continue;

            const QString name = s.left(eq).trimmed();
            const QString hex  = s.mid(eq + 1).trimmed();

            const QColor c(hex);
            if (!name.isEmpty() && c.isValid())
                colors[name] = c;
        }
    }

    _projectClasses = classes;
    _projectClassColors = colors;

    // Cинхронизируем диалог
    if (_projPropsDialog)
    {
        _projPropsDialog->setProjectClasses(_projectClasses);
        _projPropsDialog->setProjectClassColors(_projectClassColors);
    }

    return true;
}

bool MainWindow::saveProjectClasses(const QStringList& classes,
                                    const QMap<QString, QColor>& colors)
{
    const QString path = classesYamlPath();
    if (path.isEmpty())
        return false;

    QList<QString> cls;
    cls.reserve(classes.size());
    for (const QString& s : classes)
        cls.push_back(s);

    // class_colors как список строк "name=#AARRGGBB"
    QList<QString> colorLines;
    colorLines.reserve(classes.size());
    for (const QString& name : classes)
    {
        const QColor c = colors.value(name, QColor());
        if (c.isValid())
            colorLines.push_back(name + "=" + c.name(QColor::HexArgb));
    }

    YamlConfig yconfig;
    yconfig.setValue("classes", cls);
    yconfig.setValue("class_colors", colorLines);

    const QByteArray encoded = QFile::encodeName(QDir::toNativeSeparators(path));
    return yconfig.saveFile(std::string(encoded.constData(), encoded.size()));

    // std::string str;
    // yconfig.saveString(str);

    // // YAML::Emitter seq;
    // // seq.SetIndent(2);
    // // seq << YAML::Flow << YAML::BeginSeq;
    // // for (const QString& s : classes)
    // //     seq << s.toUtf8().constData();
    // // seq << YAML::EndSeq;
    // // if (!seq.good())
    // //     return false;

    // // защита от "пустой записи"
    // if (str.empty())
    //     return false;


    // QByteArray body;
    // body += "---\n";
    // body += "### QLabelMe YAML syntax ###\n\n";
    // body += str.c_str();
    // body += "\n\n...\n";

    // QSaveFile sf {path};
    // if (!sf.open(QIODevice::WriteOnly))
    //     return false;

    // if (sf.write(body) != body.size())
    //     return false;

    // return sf.commit();
}

void MainWindow::onPolygonListItemClicked(QListWidgetItem* item)
{
    if (!item) return;

    QGraphicsItem* sceneItem = item->data(Qt::UserRole).value<QGraphicsItem*>();
    if (sceneItem)
    {
       ui->graphView->centerOn(sceneItem);
    }
}

void MainWindow::onPolygonListItemDoubleClicked(QListWidgetItem* item)
{
    if (!item) return;

    QGraphicsItem* graphicsItem = item->data(Qt::UserRole).value<QGraphicsItem*>();
    if (!graphicsItem) return;

    // Фокусируемся на элементе и увеличиваем масштаб
    ui->graphView->centerOn(graphicsItem);
    ui->graphView->scale(1.2, 1.2);
}

void MainWindow::onSceneSelectionChanged()
{
    // Блокируем сигналы списка, чтобы избежать рекурсии
    ui->polygonList->blockSignals(true);

    // Снимаем выделение со всех элементов в списке
    for (int i = 0; i < ui->polygonList->count(); ++i)
    {
        ui->polygonList->item(i)->setSelected(false);
    }

    // Выделяем соответствующие элементы в списке
    for (QGraphicsItem* item : _scene->selectedItems())
    {
        for (int i = 0; i < ui->polygonList->count(); ++i)
        {
            QListWidgetItem* listItem = ui->polygonList->item(i);
            if (listItem->data(Qt::UserRole).value<QGraphicsItem*>() == item)
            {
                listItem->setSelected(true);
                ui->polygonList->scrollToItem(listItem);
                break;
            }
        }
    }
    // Разблокируем сигналы списка
    ui->polygonList->blockSignals(false);
    updateCoordinateList();
    raiseAllHandlesToTop();
}

void MainWindow::updateCoordinateList()
{
    if (!ui || !ui->coordinateList)
        return;

    ui->coordinateList->clear();

    if (!_scene)
        return;

    QList<QGraphicsItem*> selectedItems = _scene->selectedItems();
    if (selectedItems.isEmpty())
        return;

    auto addLine = [this](const QString& text)
    {
        ui->coordinateList->addItem(text);
    };

    auto calcBounds = [](const QVector<QPointF>& pts,
                         QPoint& topLeft,
                         QPoint& bottomRight,
                         QPoint& center) -> bool
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

        topLeft = QPoint(static_cast<int>(std::round(minX)),
                         static_cast<int>(std::round(minY)));
        bottomRight = QPoint(static_cast<int>(std::round(maxX)),
                             static_cast<int>(std::round(maxY)));
        center = QPoint((topLeft.x() + bottomRight.x()) / 2,
                        (topLeft.y() + bottomRight.y()) / 2);
        return true;
    };

    bool firstShape = true;

    for (QGraphicsItem* item : selectedItems)
    {
        if (!item)
            continue;

        if (auto* point = dynamic_cast<qgraph::Point*>(item))
        {
            if (!firstShape)
                addLine(QString());
            firstShape = false;

            QPoint center = point->center();

            addLine(tr("Точка"));
            addLine(tr("Центр: (%1; %2)")
                        .arg(center.x())
                        .arg(center.y()));
        }
        else if (auto* rect = dynamic_cast<qgraph::Rectangle*>(item))
        {
            if (!firstShape)
                addLine(QString());
            firstShape = false;

            QRectF r = rect->sceneBoundingRect();

            QPoint topLeft(std::round(r.topLeft().x()),
                           std::round(r.topLeft().y()));
            QPoint bottomRight(std::round(r.bottomRight().x()),
                               std::round(r.bottomRight().y()));
            QPoint center((topLeft.x() + bottomRight.x()) / 2,
                          (topLeft.y() + bottomRight.y()) / 2);

            int width  = bottomRight.x() - topLeft.x();
            int height = bottomRight.y() - topLeft.y();

            addLine(tr("Прямоугольник"));
            addLine(tr("TL точка: (%1; %2)")
                        .arg(topLeft.x())
                        .arg(topLeft.y()));
            addLine(tr("BR точка: (%1; %2)")
                        .arg(bottomRight.x())
                        .arg(bottomRight.y()));
            addLine(tr("Центр: (%1; %2)")
                        .arg(center.x())
                        .arg(center.y()));
            addLine(tr("Ширина: %1").arg(width));
            addLine(tr("Высота: %1").arg(height));
        }
        else if (auto* polyline = dynamic_cast<qgraph::Polyline*>(item))
        {
            QPoint topLeft;
            QPoint bottomRight;
            QPoint center;
            if (!calcBounds(polyline->points(), topLeft, bottomRight, center))
                continue;

            if (!firstShape)
                addLine(QString());
            firstShape = false;

            int width  = bottomRight.x() - topLeft.x();
            int height = bottomRight.y() - topLeft.y();

            addLine(tr("Полилиния"));
            addLine(tr("TL точка: (%1; %2)")
                        .arg(topLeft.x())
                        .arg(topLeft.y()));
            addLine(tr("BR точка: (%1; %2)")
                        .arg(bottomRight.x())
                        .arg(bottomRight.y()));
            addLine(tr("Центр: (%1; %2)")
                        .arg(center.x())
                        .arg(center.y()));
            addLine(tr("Ширина: %1").arg(width));
            addLine(tr("Высота: %1").arg(height));
        }
        else if (auto* line = dynamic_cast<qgraph::Line*>(item))
        {
            QPoint topLeft;
            QPoint bottomRight;
            QPoint center;
            if (!calcBounds(line->points(), topLeft, bottomRight, center))
                continue;

            if (!firstShape)
                addLine(QString());
            firstShape = false;

            int width  = bottomRight.x() - topLeft.x();
            int height = bottomRight.y() - topLeft.y();

            addLine(tr("Линия"));
            addLine(tr("TL точка: (%1; %2)")
                        .arg(topLeft.x())
                        .arg(topLeft.y()));
            addLine(tr("BR точка: (%1; %2)")
                        .arg(bottomRight.x())
                        .arg(bottomRight.y()));
            addLine(tr("Центр: (%1; %2)")
                        .arg(center.x())
                        .arg(center.y()));
            addLine(tr("Ширина: %1").arg(width));
            addLine(tr("Высота: %1").arg(height));
        }
        else if (auto* circle = dynamic_cast<qgraph::Circle*>(item))
        {
            if (!firstShape)
                addLine(QString());
            firstShape = false;

            QPoint center = circle->center();
            int radius = circle->realRadius();

            QPoint topLeft(center.x() - radius,
                           center.y() - radius);
            QPoint bottomRight(center.x() + radius,
                               center.y() + radius);

            int width  = bottomRight.x() - topLeft.x();
            int height = bottomRight.y() - topLeft.y();

            addLine(tr("Круг"));
            addLine(tr("TL точка: (%1; %2)")
                        .arg(topLeft.x())
                        .arg(topLeft.y()));
            addLine(tr("BR точка: (%1; %2)")
                        .arg(bottomRight.x())
                        .arg(bottomRight.y()));
            addLine(tr("Центр: (%1; %2)")
                        .arg(center.x())
                        .arg(center.y()));
            addLine(tr("Ширина: %1").arg(width));
            addLine(tr("Высота: %1").arg(height));
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

void MainWindow::linkSceneItemToList(QGraphicsItem* sceneItem)
{
    QString className = sceneItem->data(0).toString();
    if (className.isEmpty()) return;

    QListWidgetItem* listItem = new QListWidgetItem(className);
    listItem->setData(Qt::UserRole, QVariant::fromValue(sceneItem));
    ui->polygonList->addItem(listItem);
}

void MainWindow::showPolygonListContextMenu(const QPoint &pos)
{
    QListWidgetItem* item = ui->polygonList->itemAt(pos);
    if (!item) return;

    QMenu menu;
    QAction* deleteAction = menu.addAction("Удалить");

    if (menu.exec(ui->polygonList->viewport()->mapToGlobal(pos)) == deleteAction)
    {
        removePolygonListItem(item);
    }
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

void MainWindow::removePolygonItem(QGraphicsItem* item)
{
    if (!item) return;

    // Удаляем из списка
    for (int i = 0; i < ui->polygonList->count(); ++i)
    {
        QListWidgetItem* listItem = ui->polygonList->item(i);
        if (listItem && listItem->data(Qt::UserRole).value<QGraphicsItem*>() == item)
        {
            delete ui->polygonList->takeItem(i);
            break;
        }
    }

    // Удаляем со сцены
    if (item->scene())
    {
        item->scene()->removeItem(item);
    }
    delete item;

    // Помечаем документ как измененный
    if (auto doc = currentDocument())
    {
        doc->isModified = true;
        updateFileListDisplay(doc->filePath);
    }
}

void MainWindow::raiseAllHandlesToTop()
{
    if (!_scene) return;

    // Проходим по всем элементам и поднимаем их ручки
    for (QGraphicsItem* item : _scene->items())
    {
        if (auto* rectangle = dynamic_cast<qgraph::Rectangle*>(item))
        {
            rectangle->raiseHandlesToTop();
        }
        else if (auto* polyline = dynamic_cast<qgraph::Polyline*>(item))
        {
            polyline->raiseHandlesToTop();
        }
        else if (auto* circle = dynamic_cast<qgraph::Circle*>(item))
        {
            circle->raiseHandlesToTop();
        }
        else if (auto* point = dynamic_cast<qgraph::Point*>(item))
            point->raiseHandlesToTop();
    }
}

void MainWindow::moveSelectedItemsToBack()
{
    QList<QGraphicsItem*> selectedItems = _scene->selectedItems();
    for (QGraphicsItem* item : selectedItems)
    {
        if (auto* rectangle = dynamic_cast<qgraph::Rectangle*>(item))
        {
            rectangle->moveToBack();
        }
        else if (auto* polyline = dynamic_cast<qgraph::Polyline*>(item))
        {
            polyline->moveToBack();
        }
        else if (auto* circle = dynamic_cast<qgraph::Circle*>(item))
        {
            circle->moveToBack();
        }
    }
    // Обновляем ручки после перемещения
    raiseAllHandlesToTop();
}

bool MainWindow::hasUnsavedChanges() const
{
    for (const auto& doc : _documentsMap.values())
    {
        if (doc && doc->isModified)
            return true;
    }
    return false;
}

QList<Document::Ptr> MainWindow::getUnsavedDocuments() const
{
    QList<Document::Ptr> unsavedDocs;
    for (auto it = _documentsMap.constBegin(); it != _documentsMap.constEnd(); ++it)
    {
        if (it.value()->isModified)
        {
            unsavedDocs.append(it.value());
        }
    }
    return unsavedDocs;
}

int MainWindow::showUnsavedChangesDialog(const QList<Document::Ptr> &unsavedDocs)
{
    //QDialog dialog(this);
    QDialog dialog(
        this,
        Qt::Dialog |
            Qt::CustomizeWindowHint |
            Qt::WindowTitleHint |
            Qt::WindowCloseButtonHint
        );

    dialog.setWindowTitle(tr("Несохраненные изменения"));
    dialog.setModal(true);
    //dialog.setMinimumWidth(500);


    dialog.setWindowFlag(Qt::WindowMaximizeButtonHint, false);
    dialog.setWindowFlag(Qt::WindowMinMaxButtonsHint, false);
    dialog.setWindowFlag(Qt::WindowFullscreenButtonHint, false);
    dialog.setWindowState(Qt::WindowNoState);

    //dialog.setWindowState(Qt::WindowNoState);

    QVector<int> dlgGeom {100, 100, 600, 400};
    config::base().getValue("windows.unsaved_changes_dialog.geometry", dlgGeom);
    dialog.setGeometry(dlgGeom[0], dlgGeom[1], dlgGeom[2], dlgGeom[3]);
    //dialog.setFixedSize(dialog.size());

    QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);

    QLabel *messageLabel = new QLabel(
        tr("Следующие файлы имеют несохраненные изменения:\nХотите сохранить изменения?"));
    mainLayout->addWidget(messageLabel);

    // Список файлов
    QListWidget *fileList = new QListWidget();
    for (const Document::Ptr &doc : unsavedDocs)
    {
        QFileInfo fileInfo(doc->filePath);
        QListWidgetItem *item = new QListWidgetItem(fileInfo.fileName());
        //item->setData(Qt::UserRole, QVariant::fromValue(doc));
        item->setData(Qt::UserRole, doc->filePath);
        fileList->addItem(item);
        item->setSelected(true);
    }
    mainLayout->addWidget(fileList);

    // Кнопки
    QDialogButtonBox *buttonBox = new QDialogButtonBox();

    QPushButton *saveAllButton = new QPushButton(tr("Сохранить все"));
    QPushButton *discardAllButton = new QPushButton(tr("Не сохранять"));
    QPushButton *cancelButton = new QPushButton(tr("Отмена"));

    buttonBox->addButton(saveAllButton, QDialogButtonBox::AcceptRole);
    buttonBox->addButton(discardAllButton, QDialogButtonBox::DestructiveRole);
    buttonBox->addButton(cancelButton, QDialogButtonBox::RejectRole);

    mainLayout->addWidget(buttonBox);

    int result = QDialog::Rejected;

    connect(saveAllButton, &QPushButton::clicked, [&]() {
        result = QDialogButtonBox::SaveAll;
        dialog.accept();
    });

    connect(discardAllButton, &QPushButton::clicked, [&]() {
        result = QDialogButtonBox::Discard;
        dialog.accept();
    });

    connect(cancelButton, &QPushButton::clicked, [&]() {
        result = QDialogButtonBox::Cancel;
        dialog.reject();
    });

    dialog.exec();
    // Сохраняем геометрию только если окно не было развернуто на весь экран
    if (!dialog.isMaximized() && !dialog.isFullScreen())
    {
        QRect g = dialog.geometry();
        QVector<int> v {g.x(), g.y(), g.width(), g.height()};
        config::base().setValue("windows.unsaved_changes_dialog.geometry", v);
    }
    return result;
}

void MainWindow::saveAllDocuments()
{
    for (auto it = _documentsMap.begin(); it != _documentsMap.end(); ++it)
    {
        if (it.value()->isModified)
        {
            saveAnnotationToFile(it.value());
        }
    }
}

qgraph::DragCircle* MainWindow::pickHiddenHandle(const QPointF& scenePos, bool& topIsHandle) const
{
    topIsHandle = false;
    if (!_scene) return nullptr;

    // Берем все элементы под курсором в окрестности
    const QRectF pickRect(scenePos - QPointF(_ghostPickRadius, _ghostPickRadius),
                          QSizeF(2 * _ghostPickRadius, 2 * _ghostPickRadius));

    const auto items = _scene->items(pickRect, Qt::IntersectsItemShape, Qt::DescendingOrder);

    // Если верхний элемент уже ручка - «обманка» не нужна, пусть работает стандартный drag
    if (!items.isEmpty())
        if (auto* dc = dynamic_cast<qgraph::DragCircle*>(items.front()))
        {
            topIsHandle = true;
            return dc;
        }

    // Ищем ближайшую ручку в глубине стека
    qgraph::DragCircle* best = nullptr;
    qreal bestD2 = std::numeric_limits<qreal>::max();

    for (auto* it : items)
    {
        if (auto* circle = dynamic_cast<qgraph::Circle*>(it))
        {
            QPointF localCursorPos = circle->mapFromScene(scenePos);
            if (circle->isCursorNearCircle(localCursorPos))
            {
                auto* handle = circle->getDragCircle();
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
        if (auto* dc = dynamic_cast<qgraph::DragCircle*>(it))
        {
            const QPointF c = dc->mapToScene(dc->rect().center());
            const qreal dx = c.x() - scenePos.x();
            const qreal dy = c.y() - scenePos.y();
            const qreal d2 = dx*dx + dy*dy;
            if (d2 <= bestD2)
            {
                bestD2 = d2;
                best = dc;
            }
        }
    }
    return best;
}

void MainWindow::ensureGhostAllocated()
{
    if (!_ghostHandle)
    {
        _ghostHandle = new QGraphicsRectItem();
    }
    if (_ghostHandle->scene() != _scene)
    {
        if (_ghostHandle->scene())
            _ghostHandle->scene()->removeItem(_ghostHandle);
        if (_scene)
            _scene->addItem(_ghostHandle);
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

    QColor ghostColor = _vis.selectedHandleColor;
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
        //_ghostHandle->setPos(scenePos);
        if (_ghostActive)
            setGhostStyleHover();
    }
    _ghostTarget->setPos(newLocalTopLeft); // это триггерит itemChange() -> dragCircleMove()
}

void MainWindow::endGhost()
{
    if (_ghostTarget)
    {
        _ghostTarget->setHoverStyle(false);
    }
    if (_lastHoverHandle)
    {
        _ghostTarget->setHoverStyle(false);
        _lastHoverHandle = nullptr;
    }

    if (_ghostTarget)
    {
        if (auto* shape = dynamic_cast<qgraph::Shape*>(_ghostTarget->parentItem()))
            shape->dragCircleRelease(_ghostTarget);
        _ghostTarget->setHoverStyle(false);
        _ghostTarget->setGhostDriven(false);

        // Помечаем документ как измененный после завершения перетаскивания
        Document::Ptr doc = currentDocument();
        if (doc && !doc->isModified)
        {
            doc->isModified = true;
            updateFileListDisplay(doc->filePath);
        }
    }

    _ghostActive = false;
    _ghostHover  = false;
    _ghostTarget = nullptr;
    _handleDragging = false; // Завершение перетаскивания

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
    //const qreal scale = 1;
    const QSizeF newSize(base.size() * scale);
    const QRectF newRect(QPointF(-newSize.width()/2.0, -newSize.height()/2.0), newSize);

    QColor ghostColor = _vis.selectedHandleColor;
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
    if (!_ghostTarget || !_ghostHandle) return;

    _ghostActive = true;
    _ghostActive = true;
    _ghostHover  = false;

    _ghostGrabOffset = scenePos - _ghostHandle->pos();
    _ghostTarget->setGhostDriven(true);
    setGhostStyleHover();
}

qgraph::DragCircle* MainWindow::pickHandleAt(const QPointF& scenePos) const
{
    if (!_scene) return nullptr;

    const qreal pickRadius = 10.0; // Радиус поиска
    const QRectF probe(scenePos - QPointF(pickRadius, pickRadius),
                       QSizeF(pickRadius * 2, pickRadius * 2));

    for (QGraphicsItem* item : _scene->items(probe, Qt::IntersectsItemShape))
    {
        // Проверяем, что элемент все еще существует и валиден
        if (!item || !item->scene()) continue;

        if (auto* circle = dynamic_cast<qgraph::Circle*>(item))
        {
            // Для кругов всегда возвращаем их ручку, если курсор рядом с окружностью
            if (circle->isCursorNearCircle(circle->mapFromScene(scenePos)))
            {
                // Получаем ручку круга и обновляем ее позицию
                auto* handle = circle->getDragCircle();
                if (handle && handle->isValid())
                {
                    circle->updateHandlePosition(scenePos);
                    return handle;
                }
            }
            continue;
        }

        if (auto* handle = dynamic_cast<qgraph::DragCircle*>(item))
        {
            if (!handle->isValid())
                continue;
            // Для других фигур используем стандартную проверку
            if (handle->contains(handle->mapFromScene(scenePos)))
            {
                return handle;
            }
        }
    }
    return nullptr;
}

void MainWindow::startHandleDrag(qgraph::DragCircle* h, const QPointF& scenePos)
{    
    m_isDraggingHandle = true;
    m_dragHandle = h;
    _handleDragging = true; // Начало перетаскивания ручки

    // UNDO: запомним владельца и снимок "до"
    _handleEditedItem = h ? h->parentItem() : nullptr;
    _handleDragHadChanges = false;
    if (_handleEditedItem)
    {
        _handleBeforeSnap = makeBackupFromItem(_handleEditedItem);
    }

    const QPointF handleCenter = h->sceneBoundingRect().center();
    m_pressLocalOffset = scenePos - handleCenter;
    h->setHoverStyle(true);
}

void MainWindow::updateHandleDrag(const QPointF& scenePos)
{
    if (!m_dragHandle) return;

    QGraphicsItem* parent = m_dragHandle->parentItem();
    QPointF parentPos = parent
                            ? parent->mapFromScene(scenePos - m_pressLocalOffset)
                            : (scenePos - m_pressLocalOffset);

    m_dragHandle->setPos(parentPos);

    emit m_dragHandle->dragCircleMove(m_dragHandle->index(),
                                      m_dragHandle->scenePos());
    updateAllPointNumbers();
    _handleDragHadChanges = true;
    updateCoordinateList();
}

void MainWindow::finishHandleDrag()
{
    if (m_dragHandle)
    {
        emit m_dragHandle->dragCircleRelease(m_dragHandle->index(),
                                             m_dragHandle->scenePos());

        // Помечаем документ как измененный после завершения перетаскивания
        Document::Ptr doc = currentDocument();
        if (doc && !doc->isModified)
        {
            doc->isModified = true;
            updateFileListDisplay(doc->filePath);
        }
        m_dragHandle->restoreBaseStyle();
    }
    // UNDO: если что-то реально поменяли - пушим команду
    if (_handleEditedItem && _handleDragHadChanges)
    {
        ShapeBackup after = makeBackupFromItem(_handleEditedItem);
        if (!sameGeometry(_handleBeforeSnap, after))
        {
            // красивое имя действия
            QString what = tr("Перемещение узла");
            if (m_dragHandle) {
                what += tr(" #1").arg(m_dragHandle->index());
            }
            pushHandleEditCommand(_handleEditedItem, _handleBeforeSnap, after, what);
        }
    }
    _handleEditedItem = nullptr;
    _handleDragHadChanges = false;

    m_isDraggingHandle = false;
    m_dragHandle = nullptr;
    _handleDragging = false; // Завершение перетаскивания ручки
    updateAllPointNumbers();
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

void MainWindow::showGhostFor(qgraph::DragCircle* h)
{
    if (!h) return;
    ensureGhostAllocated();

    _ghostTarget = h;
    _ghostHover  = true;
    _ghostActive = false;

    // Для круга используем специальную логику позиционирования
    if (auto* circle = dynamic_cast<qgraph::Circle*>(h->parentItem()))
    {
        // Получаем позицию курсора в сцене
        QPointF cursorPos = ui->graphView->mapToScene(ui->graphView->mapFromGlobal(QCursor::pos()));

        // Проверяем, что курсор действительно рядом с окружностью
        QPointF localCursorPos = circle->mapFromScene(cursorPos);
        if (circle->isCursorNearCircle(localCursorPos))
        {
            // Обновляем позицию реальной ручки
            circle->updateHandlePosition(cursorPos);

            const QPointF sceneHandlePos = h->scenePos();
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
        const QPointF centerScene = h->sceneBoundingRect().center();
        _ghostHandle->setPos(centerScene - _ghostHandle->rect().center());
    }

    const QPointF centerScene = h->sceneBoundingRect().center();
    const QRectF base = _ghostTarget->baseRect();
    const qreal scale = 1.5;
    //const qreal scale = 1;
    const QSizeF newSize(base.size() * scale);
    const QRectF newRect(QPointF(-newSize.width()/2.0, -newSize.height()/2.0), newSize);

    QColor ghostColor = _vis.selectedHandleColor;
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
    if (!_scene) return;

    for (QGraphicsItem* item : _scene->items())
    {
        if (auto* rectangle = dynamic_cast<qgraph::Rectangle*>(item))
        {
            rectangle->updatePointNumbers();
        }
        else if (auto* polyline = dynamic_cast<qgraph::Polyline*>(item))
        {
            polyline->updatePointNumbers();
        }
        else if (auto* line = dynamic_cast<qgraph::Line*>(item))
        {
            line->updatePointNumbers();
        }
    }
}

void MainWindow::loadVisualStyle()
{
    // Устанавливаем значения по умолчанию, если они не найдены в конфиге
    if (!config::base().getValue("graphics.line_width", _vis.lineWidth))
        _vis.lineWidth = 2.0;

    if (!config::base().getValue("graphics.handle_size", _vis.handleSize))
        _vis.handleSize = 10.0;

    if (!config::base().getValue("graphics.number_font_pt", _vis.numberFontPt))
        _vis.numberFontPt = 10.0;

    if (!config::base().getValue("graphics.point_size", _vis.pointSize))
        _vis.pointSize = 6;

    // Загружаем цвета
    QString colorStr;
    if (config::base().getValue("graphics.handle_color", colorStr))
        _vis.handleColor = QColor(colorStr);
    else
        _vis.handleColor = Qt::red;

    if (config::base().getValue("graphics.number_color", colorStr))
        _vis.numberColor = QColor(colorStr);
    else
        _vis.numberColor = Qt::white;

    if (config::base().getValue("graphics.number_bg_color", colorStr))
        _vis.numberBgColor = QColor(colorStr);
    else
        _vis.numberBgColor = QColor(0, 0, 0, 180);

    // Загружаем цвета линий
    if (config::base().getValue("graphics.colors.rectangle", colorStr))
        _vis.rectangleLineColor = QColor(colorStr);
    else
        _vis.rectangleLineColor = Qt::green;

    if (config::base().getValue("graphics.colors.circle", colorStr))
        _vis.circleLineColor = QColor(colorStr);
    else
        _vis.circleLineColor = Qt::red;

    if (config::base().getValue("graphics.colors.polyline", colorStr))
        _vis.polylineLineColor = QColor(colorStr);
    else
        _vis.polylineLineColor = Qt::blue;

    if (config::base().getValue("graphics.all.selected_handle_color", colorStr))
        _vis.selectedHandleColor = QColor(colorStr);
    else
        _vis.selectedHandleColor = Qt::yellow;

    if (config::base().getValue("graphics.colors.line", colorStr))
        _vis.lineLineColor = QColor(colorStr);

    if (config::base().getValue("graphics.colors.point", colorStr))
        _vis.pointColor = QColor(colorStr);

    int ow = 1;
    if (config::base().getValue("graphics.point.outline_width", ow))
        _vis.pointOutlineWidth = ow;
    else
        _vis.pointOutlineWidth = 1;

    //config::base().getValue("vis.all.label_font_pt", _vis.labelFontPt);
    if (!config::base().getValue("graphics.label_font_pt", _vis.labelFontPt))
        _vis.labelFontPt = 0;

    QString font;
    if (config::base().getValue("graphics.label_font_family", font))
        _vis.labelFont = font;
    else
        _vis.labelFont.clear();


    applyStyle_AllDocuments();
}


void MainWindow::saveVisualStyle() const
{
    config::base().setValue("graphics.line_width", _vis.lineWidth);
    config::base().setValue("graphics.handle_size", _vis.handleSize);
    config::base().setValue("graphics.number_font_pt", _vis.numberFontPt);
    config::base().setValue("graphics.point_size", _vis.pointSize);
    config::base().setValue("graphics.handle_color", _vis.handleColor.name(QColor::HexArgb));
    config::base().setValue("graphics.selected_handle_color", _vis.selectedHandleColor.name(QColor::HexArgb));
    config::base().setValue("graphics.number_color", _vis.numberColor.name(QColor::HexArgb));
    config::base().setValue("graphics.number_bg_color", _vis.numberBgColor.name(QColor::HexArgb));
    config::base().setValue("graphics.colors.rectangle", _vis.rectangleLineColor.name(QColor::HexArgb));
    config::base().setValue("graphics.colors.circle", _vis.circleLineColor.name(QColor::HexArgb));
    config::base().setValue("graphics.colors.polyline", _vis.polylineLineColor.name(QColor::HexArgb));
    config::base().setValue("graphics.colors.line",  _vis.lineLineColor.name(QColor::HexArgb));
    config::base().setValue("graphics.colors.point", _vis.pointColor.name(QColor::HexArgb));
    config::base().setValue("graphics.point.outline_width", _vis.pointOutlineWidth);
    // config::base().setValue("vis.all.label_font_pt", _vis.labelFontPt);
    // config::base().setValue("vis.all.label_font_family", _vis.labelFont);
    if (_vis.labelFontPt > 0)
        config::base().setValue("graphics.label_font_pt", _vis.labelFontPt);

    if (!_vis.labelFont.isEmpty())
        config::base().setValue("graphics.label_font_family", _vis.labelFont);
}

void MainWindow::applyStyle_AllDocuments()
{

    for (auto it = _documentsMap.begin(); it != _documentsMap.end(); ++it)
    {
        auto doc = it.value();
        if (!doc || !doc->scene) continue;

        apply_LineWidth_ToScene(doc->scene);
        apply_PointSize_ToScene(doc->scene);
        apply_NumberSize_ToScene(doc->scene);
        updateLineColorsForScene(doc->scene);

        for (QGraphicsItem* item : doc->scene->items())
            apply_PointStyle_ToItem(item);
    }

    if (_scene)
    {
        apply_LineWidth_ToScene(_scene);
        apply_PointSize_ToScene(_scene);
        apply_NumberSize_ToScene(_scene);
        updateLineColorsForScene(_scene);

        for (QGraphicsItem* item : _scene->items())
            apply_PointStyle_ToItem(item);
    }
}

void MainWindow::forEachScene(std::function<void(QGraphicsScene*)> fn)
{
    for (auto it = _documentsMap.begin(); it != _documentsMap.end(); ++it)
    {
        auto doc = it.value();
        if (!doc) continue;
        auto *sc = doc->scene;
        if (!sc) continue;
        fn(sc);
    }
}

void MainWindow::apply_LineWidth_ToScene(QGraphicsScene* sc)
{
    if (!sc)
    {
        forEachScene([this](QGraphicsScene* scene){
            for (auto *it : scene->items()) apply_LineWidth_ToItem(it);
            scene->update();
        });
        return;
    }
    for (auto *it : sc->items()) apply_LineWidth_ToItem(it);
    sc->update();
}

void MainWindow::apply_PointSize_ToScene(QGraphicsScene* sc)
{
    if (!sc)
    {
        forEachScene([this](QGraphicsScene* scene){
            for (auto *it : scene->items()) apply_PointSize_ToItem(it);
            scene->update();
        });
        return;
    }
    for (auto *it : sc->items()) apply_PointSize_ToItem(it);
    sc->update();
}

void MainWindow::apply_NumberSize_ToScene(QGraphicsScene* sc)
{
    if (!sc)
    {
        forEachScene([this](QGraphicsScene* scene){
            for (auto *it : scene->items()) apply_NumberSize_ToItem(it);
            scene->update();
        });
        return;
    }
    for (auto *it : sc->items())
        apply_NumberSize_ToItem(it);
    sc->update();
}

void MainWindow::apply_LineWidth_ToItem(QGraphicsItem* it)
{
    if (!it) return;
    const auto w = _vis.lineWidth;

    if (auto r = dynamic_cast<qgraph::Rectangle*>(it))
    {
        QPen p=r->pen();
        p.setWidthF(w);
        //p.setColor(_vis.rectangleLineColor);
        p.setCosmetic(true);

        const QString cls = r->data(0).toString();
        const QColor cc = classColorFor(cls);
        p.setColor(cc.isValid() ? cc : _vis.rectangleLineColor);

        r->setPen(p);
        return;
    }
    if (auto c = dynamic_cast<qgraph::Circle*>(it))
    {
        QPen p=c->pen();
        p.setWidthF(w);
        //p.setColor(_vis.circleLineColor);
        p.setCosmetic(true);

        const QString cls = c->data(0).toString();
        const QColor cc = classColorFor(cls);
        p.setColor(cc.isValid() ? cc : _vis.circleLineColor);

        c->setPen(p);
        c->applyLineStyle(w);
        return;
    }
    if (auto pl= dynamic_cast<qgraph::Polyline*>(it))
    {
        QPen p=pl->pen();
        p.setWidthF(w);
        //p.setColor(_vis.polylineLineColor);
        p.setCosmetic(true);

        const QString cls = pl->data(0).toString();
        const QColor cc = classColorFor(cls);
        p.setColor(cc.isValid() ? cc : _vis.polylineLineColor);

        pl->setPen(p);
        return;
    }
    if (auto l = dynamic_cast<qgraph::Line*>(it))
    {
        QPen p = l->pen();
        p.setWidthF(_vis.lineWidth);
        //p.setColor(_vis.lineLineColor);
        p.setCosmetic(true);

        const QString cls = l->data(0).toString();
        const QColor cc = classColorFor(cls);
        p.setColor(cc.isValid() ? cc : _vis.lineLineColor);

        l->setPen(p);
        return;
    }
    if (auto pnt = dynamic_cast<qgraph::Point*>(it))
    {
        QPen p = pnt->pen();
        p.setWidthF(_vis.pointOutlineWidth); // Отдельная толщина для точки
        p.setCosmetic(true);

        const QString cls = pnt->data(0).toString();
        const QColor cc = classColorFor(cls);
        p.setColor(cc.isValid() ? cc : _vis.pointOutlineWidth);

        pnt->setPen(p);
        return;
    }

    // Базовые Qt‑элементы
    if (auto l = qgraphicsitem_cast<QGraphicsLineItem*>(it))
    {
        QPen p=l->pen();
        p.setWidthF(w);
        p.setCosmetic(true);
        l->setPen(p);
        return;
    }

    if (auto pth=qgraphicsitem_cast<QGraphicsPathItem*>(it))
    {
        QPen p=pth->pen();
        p.setWidthF(w);
        p.setCosmetic(true);
        pth->setPen(p);
        return;
    }
    if (auto el = qgraphicsitem_cast<QGraphicsEllipseItem*>(it))
    {
        QPen p=el->pen();
        p.setWidthF(w);
        p.setCosmetic(true);
        el->setPen(p);
        return;
    }
    if (auto rc = qgraphicsitem_cast<QGraphicsRectItem*>(it))
    {
        QPen p=rc->pen();
        p.setWidthF(w);
        p.setCosmetic(true);
        rc->setPen(p);
        return;
    }
}

void MainWindow::apply_PointSize_ToItem(QGraphicsItem* it)
{
    if (!it) return;
    // Ручки обычно лежат детьми нужной фигуры
    for (QGraphicsItem* ch : it->childItems())
    {
        if (auto h = dynamic_cast<qgraph::DragCircle*>(ch))
        {
            const bool isPoint = (dynamic_cast<qgraph::Point*>(it) != nullptr);

            // Размер узла для Point должен следовать размеру точки
            const qreal pointHandleSize = std::max<qreal>(_vis.pointSize, 4.0);

            h->setBaseStyle(_vis.handleColor, isPoint ? pointHandleSize : _vis.handleSize);
            h->setSelectedHandleColor(_vis.selectedHandleColor);
            h->restoreBaseStyle();
            qgraph::DragCircle::rememberCurrentAsBase(h);
        }
    }
}

void MainWindow::apply_NumberSize_ToItem(QGraphicsItem* it)
{
    if (!it) return;

    // Для классов фигур
    if (auto* rectangle = dynamic_cast<qgraph::Rectangle*>(it))
    {
        rectangle->applyNumberStyle(_vis.numberFontPt, _vis.numberColor, _vis.numberBgColor);
        return;
    }
    if (auto* polyline = dynamic_cast<qgraph::Polyline*>(it))
    {
        polyline->applyNumberStyle(_vis.numberFontPt, _vis.numberColor, _vis.numberBgColor);
        return;
    }
    if (auto* line = dynamic_cast<qgraph::Line*>(it))
    {
        line->applyNumberStyle(_vis.numberFontPt, _vis.numberColor, _vis.numberBgColor);
        return;
    }

    // Для стандартных Qt элементов
    QVector<QGraphicsSimpleTextItem*> texts;
    QVector<QGraphicsRectItem*> backgrounds;

    for (QGraphicsItem* ch : it->childItems())
    {
        if (auto t = dynamic_cast<QGraphicsSimpleTextItem*>(ch))
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
        else if (auto bg = dynamic_cast<QGraphicsRectItem*>(ch))
        {
            if (bg->zValue() > 900 && bg->zValue() < 1010)
                backgrounds << bg;
        }
    }

    // Применяем стиль к текстам
    for (auto* t : texts)
    {
        if (!t) continue;

        QFont f = t->font();
        f.setPointSizeF(_vis.numberFontPt);
        t->setFont(f);
        t->setBrush(Qt::white);
        t->setPen(Qt::NoPen);
        t->setZValue(1001);
        t->setFlag(QGraphicsItem::ItemIgnoresParentOpacity, true);
    }

    // Обновляем фоны
    for (auto* bg : backgrounds)
    {
        if (!bg) continue;

        // Ищем ближайший текст
        QGraphicsSimpleTextItem* nearest = nullptr;
        qreal bestDistance = std::numeric_limits<qreal>::max();

        for (auto* t : texts)
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

void MainWindow::apply_PointStyle_ToItem(QGraphicsItem* it)
{
    if (!it)
        return;

    if (auto* p = dynamic_cast<qgraph::Point*>(it))
    {

        const QString cls = p->data(0).toString();
        const QColor cc = classColorFor(cls);
        const QColor color = cc.isValid() ? cc : _vis.pointColor;

        const qreal diam = std::max(2, _vis.pointSize);
        p->setDotStyle(color, diam, _vis.handleSize);
    }
}

void MainWindow::applyLabelFontToUi()
{
    if (!ui || !ui->splitter)
        return;

    // Берем текущий шрифт правой зоны как базовый
    QFont f = ui->splitter->font();

    if (!_vis.labelFont.isEmpty())
        f.setFamily(_vis.labelFont);

    if (_vis.labelFontPt > 0)
        f.setPointSize(_vis.labelFontPt);

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

        if (auto* rect = dynamic_cast<qgraph::Rectangle*>(item))
        {
            QPen pen = rect->pen();
            //pen.setColor(_vis.rectangleLineColor);
            pen.setColor(hasClassColor ? classC : _vis.rectangleLineColor);
            rect->setPen(pen);
        }
        else if (auto* circle = dynamic_cast<qgraph::Circle*>(item))
        {
            QPen pen = circle->pen();
            //pen.setColor(_vis.circleLineColor);
            pen.setColor(hasClassColor ? classC : _vis.circleLineColor);
            circle->setPen(pen);
        }
        else if (auto* polyline = dynamic_cast<qgraph::Polyline*>(item))
        {
            QPen pen = polyline->pen();
            //pen.setColor(_vis.polylineLineColor);
            pen.setColor(hasClassColor ? classC : _vis.polylineLineColor);
            polyline->setPen(pen);
        }
        else if (auto* line = dynamic_cast<qgraph::Line*>(item))
        {
            QPen pen = line->pen();
            //pen.setColor(_vis.lineLineColor);
            pen.setColor(hasClassColor ? classC : _vis.lineLineColor);
            line->setPen(pen);
        }
        else if (auto* point = dynamic_cast<qgraph::Point*>(item))
        {
            QPen pen = point->pen();
            //pen.setColor(_vis.pointColor);
            pen.setColor(hasClassColor ? classC : _vis.pointColor);
            pen.setWidthF(_vis.pointOutlineWidth);
            pen.setCosmetic(true);
            point->setPen(pen);
        }
    }
    scene->update();
}

void MainWindow::applyZoom(qreal z)
{
    if (!ui->graphView) return;

    // Ограничим диапазон
    if (z < kMinZoom) z = kMinZoom;
    if (z > kMaxZoom) z = kMaxZoom;

    // Сохраним текущий центр сцены, чтобы картинка не «уезжала»
    const QPointF centerScene =
        ui->graphView->mapToScene(ui->graphView->viewport()->rect().center());

    QTransform t;
    t.scale(z, z);
    ui->graphView->setTransform(t);

    // Вернем центр
    ui->graphView->centerOn(centerScene);

    m_zoom = z;

    if (auto doc = currentDocument())
    {
        doc->viewState = {
            ui->graphView->horizontalScrollBar()->value(),
            ui->graphView->verticalScrollBar()->value(),
            m_zoom,
            centerScene
        };
    }
}

void MainWindow::applyClosePolyline()
{
    //using SDV = Settings::Values;
    using SDM = Settings::PolylineCloseMode;
    using PLM = qgraph::Polyline::CloseMode;

    qgraph::Polyline::CloseMode mode = PLM::CtrlModifier;
    switch (_polylineCloseMode)
    {
        case SDM::DoubleClick:             mode = PLM::DoubleClick;   break;
        case SDM::SingleClickOnFirstPoint: mode = PLM::SingleClickOnFirstPoint; break;
        case SDM::CtrlModifier:            mode = PLM::CtrlModifier;            break;
        case SDM::KeyCWithoutNewPoint:     mode = PLM::KeyC;                    break;
    }
    qgraph::Polyline::setGlobalCloseMode(mode);
}

void MainWindow::applyFinishLine()
{
    //using SDV = Settings::Values;
    using SDM = Settings::LineFinishMode;
    using PLM = qgraph::Line::CloseMode;

    qgraph::Line::CloseMode mode = PLM::CtrlModifier;
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

QVector<ShapeBackup> MainWindow::collectBackupsForItems(const QList<QListWidgetItem*>& listItems) const
{
    QVector<ShapeBackup> out;
    out.reserve(listItems.size());

    for (QListWidgetItem* li : listItems)
    {
        if (!li)
            continue;
        QGraphicsItem* gi = sceneItemFromListItem(li);
        if (!gi)
            continue;

        ShapeBackup b{};

        b.uid = gi->data(RoleUid).toULongLong();
        if (b.uid == 0)
            b.uid = ensureUid(gi);

        b.className = gi->data(0).toString();
        b.z = gi->zValue();
        b.visible = gi->isVisible();

        if (auto* rect = dynamic_cast<qgraph::Rectangle*>(gi))
        {
            b.kind = ShapeKind::Rectangle;
            b.rect = rect->sceneBoundingRect();
        }
        else if (auto* c = dynamic_cast<qgraph::Circle*>(gi))
        {
            b.kind = ShapeKind::Circle;
            b.circleCenter = c->center();
            b.circleRadius = c->realRadius();
        }
        else if (auto* pl = dynamic_cast<qgraph::Polyline*>(gi))
        {
            b.kind  = ShapeKind::Polyline;
            b.points = pl->points();
            b.closed = pl->isClosed();
        }
        else if (auto* ln = dynamic_cast<qgraph::Line*>(gi))
        {
            b.kind  = ShapeKind::Line;
            b.points = ln->points();
            b.closed = ln->isClosed();
        }
        else if (auto* p = dynamic_cast<qgraph::Point*>(gi))
        {
            b.kind = ShapeKind::Point;
            b.pointCenter = p->center();
        }
        else
        {
            continue;
        }

        out.push_back(std::move(b));
    }

    return out;
}

void MainWindow::removeListEntryBySceneItem(QGraphicsItem* sceneItem)
{
    if (!sceneItem)
        return;
    for (int row = 0; row < ui->polygonList->count(); ++row)
    {
        QListWidgetItem* it = ui->polygonList->item(row);
        if (!it)
            continue;
        QGraphicsItem* linked = sceneItemFromListItem(it);
        if (linked == sceneItem)
        {
            delete ui->polygonList->takeItem(row);
            return;
        }
    }
}

QGraphicsItem* MainWindow::sceneItemFromListItem(const QListWidgetItem* it)
{
    if (!it)
        return nullptr;
    return it->data(Qt::UserRole).value<QGraphicsItem*>();
}

QGraphicsItem* MainWindow::findItemByUid(qulonglong uid) const
{
    if (!_scene || uid == 0)
        return nullptr;

    const auto itemsList = _scene->items();
    for (QGraphicsItem* it : itemsList)
    {
        if (!it)
            continue;

        if (it->data(RoleUid).toULongLong() == uid)
            return it;
    }
    return nullptr;
}

qulonglong MainWindow::ensureUid(QGraphicsItem* it) const
{
    if (!it)
        return 0;
    QVariant v = it->data(RoleUid);
    if (!v.isNull())
        return v.toULongLong();
    qulonglong uid = _uidCounter++;
    it->setData(RoleUid, QVariant::fromValue<qulonglong>(uid));
    return uid;
}

void MainWindow::cancelRulerMode()
{
    _isDrawingRuler = false;
    _drawingRuler = false;

    if (_rulerLine)
    {
        _scene->removeItem(_rulerLine);
        delete _rulerLine;
        _rulerLine = nullptr;
    }

    if (_rulerText)
    {
        _scene->removeItem(_rulerText);
        delete _rulerText;
        _rulerText = nullptr;
    }
}

void MainWindow::toggleMenuBarVisible()
{
    if (!ui || !ui->menuBar)
        return;

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

    const auto it = _projectClassColors.find(className);
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

    const QColor c = classColorFor(className);
    if (!c.isValid())
        return;

    if (auto* r = dynamic_cast<qgraph::Rectangle*>(item))
    {
        QPen p = r->pen();
        p.setColor(c);
        p.setCosmetic(true);
        r->setPen(p);
        return;
    }
    if (auto* cir = dynamic_cast<qgraph::Circle*>(item))
    {
        QPen p = cir->pen();
        p.setColor(c);
        p.setCosmetic(true);
        cir->setPen(p);
        return;
    }
    if (auto* pl = dynamic_cast<qgraph::Polyline*>(item))
    {
        QPen p = pl->pen();
        p.setColor(c);
        p.setCosmetic(true);
        pl->setPen(p);
        return;
    }
    if (auto* l = dynamic_cast<qgraph::Line*>(item))
    {
        QPen p = l->pen();
        p.setColor(c);
        p.setCosmetic(true);
        l->setPen(p);
        return;
    }
    if (auto* pnt = dynamic_cast<qgraph::Point*>(item))
    {
        const QColor c = classColorFor(className);
        if (!c.isValid())
            return;

        const qreal diam = std::max(2, _vis.pointSize);
        pnt->setDotStyle(c, diam, _vis.handleSize);
        return;
    }
}

void MainWindow::updateImageSizeLabel(const QSize& size)
{
    if (!_imageSizeLabel)
        return;

    if (!size.isValid())
    {
        _imageSizeLabel->setText(tr("Размер изображения: —"));
        return;
    }

    _imageSizeLabel->setText(
        tr("Размер изображения: %1 × %2")
            .arg(size.width())
            .arg(size.height())
    );
}

// Создает и пушит в стек _undoStack новую LambdaCommand,
void MainWindow::pushCreateShapeCommand(const ShapeBackup& backup, const QString& description)
{
    struct Payload
    {
        ShapeBackup  b;
        qulonglong   uid = 0;
    };

    auto payload = std::make_shared<Payload>();
    payload->b   = backup;
    payload->uid = backup.uid;

    auto redoFn = [this, payload]()
    {
        // Пытаемся найти существующий объект по uid
        QGraphicsItem* it = nullptr;
        if (payload->uid != 0)
            it = findItemByUid(payload->uid);

        if (it)
        {
            // Фигура уже есть (первый redo после push) -
            // просто приводим ее к нужному состоянию из снапшота
            applyBackupToExisting(it, payload->b);
        }
        else
        {
            // Фигуры нет - создаем по снапшоту
            it = recreateFromBackup(payload->b);
            if (it && payload->uid != 0)
            {
                it->setData(RoleUid, QVariant::fromValue<qulonglong>(payload->uid));
            }
        }

        if (auto d = currentDocument())
        {
            d->isModified = true;
            updateFileListDisplay(d->filePath);
        }
    };

    auto undoFn = [this, payload]()
    {
        if (payload->uid == 0)
            return;

        if (QGraphicsItem* it = findItemByUid(payload->uid))
        {
            _scene->removeItem(it);
            removeListEntryBySceneItem(it);
            delete it;
        }

        if (auto d = currentDocument())
        {
            d->isModified = true;
            updateFileListDisplay(d->filePath);
        }
    };

    if (auto* st = activeUndoStack())
        st->push(new LambdaCommand(redoFn, undoFn, description));
}

QUndoStack* MainWindow::activeUndoStack() const
{
    if (!_undoGroup) return nullptr;
    if (QUndoStack* st = _undoGroup->activeStack()) return st;

    // Запасной вариант - стек текущего документа
    if (auto d = currentDocument())
        return d->_undoStack ? d->_undoStack.get() : nullptr;

    return nullptr;
}

ShapeBackup MainWindow::makeBackupFromItem(QGraphicsItem* gi) const
{
    ShapeBackup b{};
    if (!gi)
        return b;

    b.uid = gi->data(RoleUid).toULongLong();
    if (b.uid == 0)
        b.uid = ensureUid(gi);

    b.className = gi->data(0).toString();
    b.z = gi->zValue();
    b.visible = gi->isVisible();

    if (auto* rect = dynamic_cast<qgraph::Rectangle*>(gi))
    {
        b.kind = ShapeKind::Rectangle;
        //b.rect = rect->sceneBoundingRect();
        b.rect = rect->mapRectToScene(rect->rect());
    }
    else if (auto* c = dynamic_cast<qgraph::Circle*>(gi))
    {
        b.kind = ShapeKind::Circle;
        b.circleCenter = c->center();
        b.circleRadius = c->realRadius();
    }
    else if (auto* pl = dynamic_cast<qgraph::Polyline*>(gi))
    {
        b.kind  = ShapeKind::Polyline;
        b.points = pl->points();
        b.closed = pl->isClosed();
    }
    else if (auto* ln = dynamic_cast<qgraph::Line*>(gi))
    {
        b.kind  = ShapeKind::Line;
        b.points = ln->points();
        b.closed = ln->isClosed();
    }
    else if (auto* p = dynamic_cast<qgraph::Point*>(gi))
    {
        b.kind = ShapeKind::Point;
        b.pointCenter = p->center();
    }

    return b;
}

void MainWindow::pushAdoptExistingShapeCommand(QGraphicsItem* createdNow,
                                               const ShapeBackup& backup,
                                               const QString& description)
{
    struct Payload
    {
        ShapeBackup  b;
        qulonglong   uid = 0;
    };

    auto payload = std::make_shared<Payload>();
    payload->b = backup;

    // Берем uid из снапшота или с самого объекта
    qulonglong uid = backup.uid;
    if (uid == 0 && createdNow)
        uid = createdNow->data(RoleUid).toULongLong();
    if (uid == 0 && createdNow)
        uid = ensureUid(createdNow);
    payload->uid = uid;

    auto redoFn = [this, payload]()
    {
        if (payload->uid == 0)
            return;

        // Пытаемся найти уже существующий объект по uid
        QGraphicsItem* item = findItemByUid(payload->uid);
        if (item)
        {
            // Фигура есть - просто приводим ее к состоянию из снапшота
            applyBackupToExisting(item, payload->b);
        }
        else
        {
            // Фигуры нет - создаем по снапшоту
            QGraphicsItem* created = recreateFromBackup(payload->b);
            if (created && payload->uid != 0)
            {
                created->setData(RoleUid,
                                 QVariant::fromValue<qulonglong>(payload->uid));
            }
        }

        if (auto d = currentDocument())
        {
            d->isModified = true;
            updateFileListDisplay(d->filePath);
        }
    };

    auto undoFn = [this, payload]()
    {
        if (payload->uid == 0)
            return;

        if (QGraphicsItem* item = findItemByUid(payload->uid))
        {
            if (auto sc = item->scene())
                sc->removeItem(item);
            removeListEntryBySceneItem(item);
            delete item;
        }

        if (auto d = currentDocument())
        {
            d->isModified = true;
            updateFileListDisplay(d->filePath);
        }
    };

    if (auto* st = activeUndoStack())
        st->push(new LambdaCommand(redoFn, undoFn, description));
}


void MainWindow::pushMoveShapeCommand(QGraphicsItem* item,
                                      const ShapeBackup& before,
                                      const ShapeBackup& after,
                                      const QString& description)
{
    struct Payload
    {
        ShapeBackup before;
        ShapeBackup after;
        qulonglong uid = 0;
    };

    auto p = std::make_shared<Payload>();
    p->before = before;
    p->after  = after;

    qulonglong uid = 0;
    if (item)
        uid = item->data(RoleUid).toULongLong();
    if (uid == 0 && item)
        uid = ensureUid(item);
    p->uid = uid;

    auto apply = [this](qulonglong uid, const ShapeBackup& snap)
    {
        QGraphicsItem* it = findItemByUid(uid);
        if (it)
        {
            applyBackupToExisting(it, snap);
            return true;
        }
        QGraphicsItem* created = recreateFromBackup(snap);
        if (created)
        {
            created->setData(RoleUid, QVariant::fromValue<qulonglong>(snap.uid ? snap.uid : uid));
            return true;
        }
        return false;
    };

    auto redoFn = [this, p, apply]()
    {
        apply(p->uid, p->after);
        if (auto d = currentDocument())
        {
            d->isModified = true;
            updateFileListDisplay(d->filePath);
        }
    };

    auto undoFn = [this, p, apply]()
    {
        apply(p->uid, p->before);
        if (auto d = currentDocument())
        {
            d->isModified = true;
            updateFileListDisplay(d->filePath);
        }
    };

    if (auto* st = activeUndoStack())
        st->push(new LambdaCommand(redoFn, undoFn, description));
}

void MainWindow::pushHandleEditCommand(QGraphicsItem* item,
                                       const ShapeBackup& before,
                                       const ShapeBackup& after,
                                       const QString& description)
{
    struct Payload
    {
        ShapeBackup before;
        ShapeBackup after;
        qulonglong uid = 0;
    };

    auto p = std::make_shared<Payload>();
    p->before = before;
    p->after  = after;

    // Сохраняем uid
    qulonglong uid = 0;
    if (item)
        uid = item->data(RoleUid).toULongLong();
    if (uid == 0 && item)
        uid = ensureUid(item);
    p->uid = uid;

    // Найти актуальный item по uid; если нет - пересоздать из снапшота
    auto apply = [this](qulonglong uid, const ShapeBackup& snap)
    {
        QGraphicsItem* it = findItemByUid(uid);
        if (it)
        {
            applyBackupToExisting(it, snap);
            return true;
        }
        // Не нашли - пересоздаем (и привязываем тот же uid)
        QGraphicsItem* created = recreateFromBackup(snap);
        if (created)
        {
            created->setData(RoleUid, QVariant::fromValue<qulonglong>(snap.uid ? snap.uid : uid));
            return true;
        }
        return false;
    };

    auto redoFn = [this, p, apply]()
    {
        apply(p->uid, p->after);
        if (auto d = currentDocument())
        {
            d->isModified = true;
            updateFileListDisplay(d->filePath);
        }
    };

    auto undoFn = [this, p, apply]()
    {
        apply(p->uid, p->before);
        if (auto d = currentDocument())
        {
            d->isModified = true;
            updateFileListDisplay(d->filePath);
        }
    };

    if (auto* st = activeUndoStack())
        st->push(new LambdaCommand(redoFn, undoFn, description));
}

void MainWindow::pushModifyShapeCommand(qulonglong uid,
                                        const ShapeBackup& before,
                                        const ShapeBackup& after,
                                        const QString& description)
{
    auto doc = currentDocument();
    if (!doc || !doc->_undoStack)
        return;

    struct Payload
    {
        qulonglong uid;
        ShapeBackup before;
        ShapeBackup after;
    };

    auto payload = std::make_shared<Payload>();
    payload->uid    = uid;
    payload->before = before;
    payload->after  = after;

    // Применяем снапшот к существующей фигуре по uid.
    // Если фигуру не нашли - просто no-op, без пересоздания.
    auto applySnap = [this](qulonglong uid, const ShapeBackup& snap)
    {
        if (QGraphicsItem* it = findItemByUid(uid))
        {
            applyBackupToExisting(it, snap);
        }
    };

    auto redoFn = [this, payload, applySnap]()
    {
        applySnap(payload->uid, payload->after);
        if (auto d = currentDocument())
        {
            d->isModified = true;
            updateFileListDisplay(d->filePath);
        }
    };

    auto undoFn = [this, payload, applySnap]()
    {
        applySnap(payload->uid, payload->before);
        if (auto d = currentDocument())
        {
            d->isModified = true;
            updateFileListDisplay(d->filePath);
        }
    };

    doc->_undoStack->push(new LambdaCommand(redoFn, undoFn, description));
}

void MainWindow::pushMoveImageCommand(const QPointF& before,
                                      const QPointF& after,
                                      const QString& description)
{
    auto doc = currentDocument();
    if (!doc || !doc->_undoStack || !_videoRect)
        return;

    if (before == after)
        return;

    struct Payload
    {
        QPointF before;
        QPointF after;
    };

    auto payload   = std::make_shared<Payload>();
    payload->before = before;
    payload->after  = after;

    auto applyPos = [this](const QPointF& pos)
    {
        if (_videoRect)
            _videoRect->setPos(pos);
    };

    auto redoFn = [this, payload, applyPos]()
    {
        applyPos(payload->after);
        if (auto d = currentDocument())
        {
            d->isModified = true;
            updateFileListDisplay(d->filePath);
        }
    };

    auto undoFn = [this, payload, applyPos]()
    {
        applyPos(payload->before);
        if (auto d = currentDocument())
        {
            d->isModified = true;
            updateFileListDisplay(d->filePath);
        }
    };

    doc->_undoStack->push(new LambdaCommand(redoFn, undoFn, description));
}

void MainWindow::removeSceneAndListItems(const QList<QListWidgetItem*>& listItems)
{
    // Удаляем все выбранные по связке из списка - сцена
    for (QListWidgetItem* li : listItems)
    {
        if (!li)
            continue;
        QGraphicsItem* gi = li->data(Qt::UserRole).value<QGraphicsItem*>();
        if (gi)
        {
            _scene->removeItem(gi);
            delete gi;
        }
        // Удаляем сам элемент списка
        delete ui->polygonList->takeItem(ui->polygonList->row(li));
    }
}

QGraphicsItem* MainWindow::recreateFromBackup(const ShapeBackup& b)
{
    QGraphicsItem* created = nullptr;

    switch (b.kind)
    {
    case ShapeKind::Rectangle:
    {
        auto* rect = new qgraph::Rectangle(_scene);
        rect->setRealSceneRect(b.rect);
        rect->updatePointNumbers();
        apply_LineWidth_ToItem(rect);
        apply_PointSize_ToItem(rect);
        apply_NumberSize_ToItem(rect);
        rect->setData(0, b.className);
        applyClassColorToItem(rect, b.className);
        rect->setZValue(b.z);
        rect->setVisible(b.visible);
        linkSceneItemToList(rect);
        created = rect;
        break;
    }
    case ShapeKind::Circle:
    {
        auto* c = new qgraph::Circle(_scene, b.circleCenter);
        c->setRealRadius(b.circleRadius);
        apply_LineWidth_ToItem(c);
        apply_PointSize_ToItem(c);
        c->setData(0, b.className);
        applyClassColorToItem(c, b.className);
        c->setZValue(b.z);
        c->setVisible(b.visible);
        c->updateHandlePosition();
        linkSceneItemToList(c);
        created = c;
        break;
    }
    case ShapeKind::Polyline:
    {
        if (b.points.isEmpty())
            break;
        auto* pl = new qgraph::Polyline(_scene, b.points.front());
        for (int i = 1; i < b.points.size(); ++i)
            pl->addPoint(b.points[i], _scene);
        if (b.closed)
            pl->closePolyline();

        apply_LineWidth_ToItem(pl);
        apply_PointSize_ToItem(pl);
        apply_NumberSize_ToItem(pl);

        pl->setData(0, b.className);
        applyClassColorToItem(pl, b.className);
        pl->setZValue(b.z);
        pl->setVisible(b.visible);
        pl->updateHandlePosition();
        linkSceneItemToList(pl);
        created = pl;
        break;
    }
    case ShapeKind::Line:
    {
        if (b.points.isEmpty())
            break;
        auto* ln = new qgraph::Line(_scene, b.points.front());
        for (int i = 1; i < b.points.size(); ++i)
            ln->addPoint(b.points[i], _scene);
        if (b.closed)
            ln->closeLine();

        apply_LineWidth_ToItem(ln);
        apply_PointSize_ToItem(ln);
        apply_NumberSize_ToItem(ln);

        ln->setData(0, b.className);
        applyClassColorToItem(ln, b.className);
        ln->setZValue(b.z);
        ln->setVisible(b.visible);
        ln->updateHandlePosition();
        linkSceneItemToList(ln);
        created = ln;
        break;
    }
    case ShapeKind::Point:
    {
        auto* pt = new qgraph::Point(_scene);
        apply_PointSize_ToItem(pt);
        apply_NumberSize_ToItem(pt);
        apply_PointStyle_ToItem(pt);
        pt->setCenter(b.pointCenter);
        pt->setData(0, b.className);
        applyClassColorToItem(pt, b.className);
        pt->setZValue(b.z);
        pt->setVisible(b.visible);
        linkSceneItemToList(pt);
        created = pt;
        break;
    }
    }

    if (created)
        created->setFocus();

    if (created)
        created->setData(RoleUid, QVariant::fromValue<qulonglong>(b.uid));

    return created;
}

void MainWindow::applyBackupToExisting(QGraphicsItem* it, const ShapeBackup& b)
{
    if (!it) return;

    switch (b.kind)
    {
    case ShapeKind::Rectangle:
        if (auto* r = qgraphicsitem_cast<qgraph::Rectangle*>(it))
        {
            r->setRealSceneRect(b.rect);
            r->updatePointNumbers();
            apply_LineWidth_ToItem(r);
            apply_PointSize_ToItem(r);
            apply_NumberSize_ToItem(r);
            r->setData(0, b.className);
            applyClassColorToItem(r, b.className);
            r->setZValue(b.z);
            r->setVisible(b.visible);
            r->updateHandlePosition();
        }
        break;

    case ShapeKind::Circle:
        if (auto* c = qgraphicsitem_cast<qgraph::Circle*>(it))
        {
            c->setRealRadius(b.circleRadius);
            apply_LineWidth_ToItem(c);
            apply_PointSize_ToItem(c);
            c->setData(0, b.className);
            applyClassColorToItem(c, b.className);
            c->setZValue(b.z);
            c->setVisible(b.visible);
            c->updateHandlePosition();
        }
        break;

    case ShapeKind::Polyline:
    {
        if (auto* pl = qgraphicsitem_cast<qgraph::Polyline*>(it))
        {
            // Полная замена узлов из снапшота
            pl->replaceScenePoints(b.points, b.closed);

            apply_LineWidth_ToItem(pl);
            apply_PointSize_ToItem(pl);
            apply_NumberSize_ToItem(pl);
            pl->setData(0, b.className);
            applyClassColorToItem(pl, b.className);
            pl->setZValue(b.z);
            pl->setVisible(b.visible);
            pl->updateHandlePosition();
        }
        break;
    }

    case ShapeKind::Line:
    {
        if (auto* ln = qgraphicsitem_cast<qgraph::Line*>(it))
        {

            ln->replaceScenePoints(b.points, b.closed);

            apply_LineWidth_ToItem(ln);
            apply_PointSize_ToItem(ln);
            apply_NumberSize_ToItem(ln);
            ln->setData(0, b.className);
            applyClassColorToItem(ln, b.className);
            ln->setZValue(b.z);
            ln->setVisible(b.visible);
            ln->updateHandlePosition();
        }
        break;
    }
    case ShapeKind::Point:
        if (auto* pt = qgraphicsitem_cast<qgraph::Point*>(it))
        {
            apply_PointStyle_ToItem(pt);
            apply_PointSize_ToItem(pt);
            apply_NumberSize_ToItem(pt);
            pt->setCenter(b.pointCenter);
            pt->setData(0, b.className);
            applyClassColorToItem(pt, b.className);
            pt->setZValue(b.z);
            pt->setVisible(b.visible);
            pt->updateHandlePosition();
        }
        break;
    }
}

void MainWindow::removePolygonListItem(QListWidgetItem* item)
{
    if (!item) return;

    // Получаем связанный графический элемент
    QGraphicsItem* sceneItem = item->data(Qt::UserRole).value<QGraphicsItem*>();
    if (sceneItem && sceneItem->scene() == _scene)
    {
        _scene->removeItem(sceneItem);
        delete sceneItem;
    }
    // Удаляем из списка
    delete ui->polygonList->takeItem(ui->polygonList->row(item));
    // Помечаем документ как измененный
    if (auto doc = currentDocument())
    {
        doc->isModified = true;
        updateFileListDisplay(doc->filePath);
    }
}

void MainWindow::fitImageToView()
{
    if (!_scene || !_videoRect || !ui->graphView)
        return;

    _scene->setSceneRect(_videoRect->boundingRect().translated(_videoRect->pos()));

    ui->graphView->resetTransform();

    ui->graphView->fitInView(_scene->sceneRect(), Qt::KeepAspectRatio);
    ui->graphView->centerOn(_scene->sceneRect().center());

    ui->graphView->horizontalScrollBar()->setValue(0);
    ui->graphView->verticalScrollBar()->setValue(0);

    if (auto doc = currentDocument())
    {
        const qreal zoom = ui->graphView->transform().m11();
        doc->viewState = {
            0,
            0,
            zoom,
            _scene->sceneRect().center()
        };
        doc->viewState.zoom = zoom;
    }
}

void MainWindow::fileList_ItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    if (!current) return;

    // При смене документа сбрасываем все указатели
    _currentHoveredHandle = nullptr;
    _ghostTarget = nullptr;
    m_dragHandle = nullptr;

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

            // Если есть изменения в сцене, сохраняем их в документе
            if (prevDoc->scene != _scene)
            {
                // Переносим все элементы (кроме videoRect) в документ предыдущего изображения
                QList<QGraphicsItem*> items = _scene->items();
                for (QGraphicsItem* item : items)
                {
                    if (item != _videoRect)
                    {
                        //_scene->removeItem(item);
                        if (item)
                        {
                            if (auto sc = item->scene())
                                sc->removeItem(item);
                        }
                        prevDoc->scene->addItem(item);
                    }
                }

                // Обновляем указатели
                prevDoc->videoRect = _videoRect;
                _videoRect->setParentItem(nullptr);
                prevDoc->scene->addItem(_videoRect);
            }
        }
    }

    // Получаем текущий документ
    QVariant currentData = current->data(Qt::UserRole);
    if (!currentData.canConvert<Document::Ptr>())
    {
        return;
    }

    Document::Ptr currentDoc = currentData.value<Document::Ptr>();
    _currentImagePath = currentDoc->filePath;

    if (currentDoc && currentDoc->_undoStack)
    {
        // Переключаем активный стек группы
        _undoGroup->setActiveStack(currentDoc->_undoStack.get());

        // И показываем его в QUndoView
        if (_undoView) _undoView->setStack(currentDoc->_undoStack.get());
    }
    else
    {
        // Нет документа - отключаемся от группы и очищаем представление
        _undoGroup->setActiveStack(nullptr);
        if (_undoView) _undoView->setStack(nullptr);
    }


    // Временно блокируем обработку изменений сцены
    _loadingNow = true;
    bool sceneWasJustCreated = false;
    // Если сцена еще не создана, загружаем изображение
    if (!currentDoc->scene)
    {
        sceneWasJustCreated = true;

        if (!currentDoc->loadImage())
            return;

        // Загружаем аннотации, если они есть
        loadAnnotationFromFile(currentDoc);
    }

    // Устанавливаем текущую сцену
    ui->graphView->setScene(currentDoc->scene);
    _scene = currentDoc->scene;
    _videoRect = currentDoc->videoRect;

    if (_videoRect && !_videoRect->pixmap().isNull())
        updateImageSizeLabel(_videoRect->pixmap().size());
    else
        updateImageSizeLabel(QSize());

    ensureGhostAllocated();

    // Подключаем сигнал changed для новой сцены
    if (currentDoc->scene)
    {
        connect(currentDoc->scene, &QGraphicsScene::selectionChanged,
                this, &MainWindow::onSceneSelectionChanged,
                Qt::UniqueConnection);

        connect(currentDoc->scene, &QGraphicsScene::changed,
                this, &MainWindow::onSceneChanged,
                Qt::UniqueConnection);
    }

    // _loadingNow = false;
    // fitImageToView();
    // updatePolygonListForCurrentScene();

    _loadingNow = false;

    bool keepPerImageZoom = false;
    config::base().getValue("view.keep_image_scale_per_image", keepPerImageZoom);

    if (sceneWasJustCreated)
    {
        fitImageToView();
    }
    else
    {
        if (keepPerImageZoom)
            restoreViewState(currentDoc);
        else
            fitImageToView();
    }

    updatePolygonListForCurrentScene();


    clearAllHandleHoverEffects();
    // Обновляем отображение звездочки (файл изменен)
    updateFileListDisplay(currentDoc->filePath);
    raiseAllHandlesToTop();
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

    // Обновляем координаты для выделенной фигуры
    if (_leftMouseButtonDown)
    {
        updateCoordinateList();
    }

    // Document::Ptr doc = currentDocument();
    // if (!doc || doc->isModified) return;

    // // Любое изменение сцены помечает документ как измененный
    // doc->isModified = true;
    // updateFileListDisplay(doc->filePath);
}

void MainWindow::onPolygonListSelectionChanged()
{
    if (!_scene)
        return;
    QList<QListWidgetItem*> selected = ui->polygonList->selectedItems();

    _scene->blockSignals(true);
    for (QGraphicsItem* it : _scene->items())
    {
        if (it == _videoRect || it == _tempRectItem || it == _tempCircleItem || it == _tempPolyline)
            continue;
        if (it->isSelected())
            it->setSelected(false);
    }
    for (auto* li : selected)
    {
        if (!li)
            continue;
        if (QGraphicsItem* scItem = li->data(Qt::UserRole).value<QGraphicsItem*>())
            scItem->setSelected(true);
    }
    _scene->blockSignals(false);

    updateCoordinateList();

    if (!selected.isEmpty())
    {
        if (QGraphicsItem* scItem = selected.first()->data(Qt::UserRole).value<QGraphicsItem*>())
            ui->graphView->ensureVisible(scItem);
    }
    raiseAllHandlesToTop();
}

void MainWindow::on_actRect_triggered()
{
    _btnRectFlag = true;

    _drawingRectangle = true;
    _drawingCircle = false;
    _drawingPolyline = false;
    _drawingLine = false;
    _drawingPoint = false;
}

void MainWindow::on_actCircle_triggered()
{
    _btnCircleFlag = true;

    _drawingCircle = true;
    _drawingPolyline = false;
    _drawingRectangle = false;
    _drawingLine = false;
    _drawingPoint = false;

}

void MainWindow::on_actPolyline_triggered()
{
    _btnPolylineFlag = true;

    _drawingPolyline = true;
    _drawingRectangle = false;
    _drawingCircle = false;
    _drawingLine = false;
    _drawingPoint = false;
}

void MainWindow::on_actPoint_triggered()
{
    _btnPointFlag = true;

    _drawingPoint = true;
    _drawingRectangle = false;
    _drawingCircle = false;
    _drawingPolyline = false;
    _drawingLine = false;
}


void MainWindow::on_actLine_triggered()
{
    _btnLineFlag = true;

    _drawingLine = true;
    _drawingPoint = false;
    _drawingRectangle = false;
    _drawingCircle = false;
    _drawingPolyline = false;
}

void MainWindow::on_actRuler_triggered()
{
    // Отключаем другие режимы рисования
    _drawingLine      = false;
    _drawingPoint     = false;
    _drawingRectangle = false;
    _drawingCircle    = false;
    _drawingPolyline  = false;

    // Включаем режим линейки
    _drawingRuler     = true;
    _isDrawingRuler   = false;
    //_isInDrawingMode  = true;
    //setSceneItemsMovable(false);

    // При включении инструмента очищаем старую линейку
    if (_rulerLine)
    {
        _scene->removeItem(_rulerLine);
        delete _rulerLine;
        _rulerLine = nullptr;
    }
    if (_rulerText)
    {
        _scene->removeItem(_rulerText);
        delete _rulerText;
        _rulerText = nullptr;
    }
}

void MainWindow::on_actClosePolyline_triggered()
{
    // Завершение рисования полилинии, если мы ее сейчас рисуем
    if (_drawingPolyline && _polyline)
    {
        _drawingPolyline = false;
        _polyline->closePolyline();

        _polyline->updatePointNumbers();
        apply_LineWidth_ToItem(_polyline);
        apply_PointSize_ToItem(_polyline);
        apply_NumberSize_ToItem(_polyline);

        // Выбор класса
        if (!_resumeEditing)
        {
            SelectClass dialog(_projectClasses, this);
            if (dialog.exec() == QDialog::Accepted)
            {
                const QString selectedClass = dialog.selectedClass();
                if (!selectedClass.isEmpty())
                {
                    _polyline->setData(0, selectedClass);
                    applyClassColorToItem(_polyline, selectedClass);
                    linkSceneItemToList(_polyline);

                    ShapeBackup b = makeBackupFromItem(_polyline);
                    pushAdoptExistingShapeCommand(_polyline, b, tr("Добавление полилинии"));
                }
            }
        }

        _polyline = nullptr;

        if (auto doc = currentDocument())
        {
            doc->isModified = true;
            updateFileListDisplay(doc->filePath);
        }
        raiseAllHandlesToTop();
    }

    // Завершение рисования линии, если мы ее сейчас рисуем
    if (_drawingLine && _line)
    {
        _drawingLine = false;
        _line->closeLine();

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
                    _line->setData(0, selectedClass);
                    applyClassColorToItem(_line, selectedClass);
                    linkSceneItemToList(_line);

                    ShapeBackup b = makeBackupFromItem(_line);
                    pushAdoptExistingShapeCommand(_line, b, tr("Добавление линии"));
                }
            }
        }

        _line = nullptr;

        if (auto doc = currentDocument())
        {
            doc->isModified = true;
            updateFileListDisplay(doc->filePath);
        }
        raiseAllHandlesToTop();
    }
}

void MainWindow::wheelEvent(QWheelEvent* event)
{
    // Сохраняем состояние при изменении масштаба
    if (auto doc = currentDocument())
    {
        saveCurrentViewState(doc);
    }
    QMainWindow::wheelEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    // При изменении размера сохраняем текущее состояние
    if (auto doc = currentDocument())
    {
        saveCurrentViewState(doc);
        QMainWindow::resizeEvent(event);
        restoreViewState(doc);
    }
    QMainWindow::resizeEvent(event);
}

void MainWindow::onCheckBoxPolygonLabel(QAbstractButton *button)
{
    QCheckBox *checkBox = qobject_cast<QCheckBox*>(button);
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
        QMessageBox::warning(this, tr("Ошибка"), tr("Выбранная папка не существует!"));
        return;
    }

    _currentFolderPath = folderPath;
    _lastUsedFolder = folderPath;
    saveLastUsedFolder();

    updateWindowTitle();
    updateFolderPathDisplay();
    loadFilesFromFolder(folderPath);
    //loadClassesFromFile(folderPath + "/classes.yaml");
    QDir dir(folderPath);
    loadClassesFromFile(dir.filePath("classes.yaml"));
}

void MainWindow::onSceneItemRemoved(QGraphicsItem* item)
{
    // Пропускаем временные элементы и само изображение
    if (item == _videoRect || item == _tempRectItem ||
        item == _tempCircleItem || item == _tempPolyline)
    {
        return;
    }

    // Ищем соответствующий элемент в списке полигонов
    for (int i = 0; i < ui->polygonList->count(); ++i)
    {
        QListWidgetItem* listItem = ui->polygonList->item(i);
        QVariant itemData = listItem->data(Qt::UserRole);

        if (itemData.isValid() && itemData.value<QGraphicsItem*>() == item)
        {
            delete ui->polygonList->takeItem(i);
            break;
        }
    }
}

void MainWindow::on_actDelete_triggered()
{
    auto doc = currentDocument();
    if (!doc)
        return;

    const QList<QListWidgetItem*> selectedItems = ui->polygonList->selectedItems();
    if (selectedItems.isEmpty())
        return;

    // Снимки для восстановления
    const QVector<ShapeBackup> backups = collectBackupsForItems(selectedItems);

    struct Payload
    {
        QVector<ShapeBackup>  backups;
        QVector<qulonglong>   uids;
    };

    auto payload = std::make_shared<Payload>();
    payload->backups = backups;
    payload->uids.reserve(backups.size());

    for (const ShapeBackup& b : backups)
    {
        if (b.uid != 0)
            payload->uids.push_back(b.uid);
    }

    // redo: удалить текущие элементы по uid
    auto redoFn = [this, payload]()
    {
        for (qulonglong uid : std::as_const(payload->uids))
        {
            if (!uid)
                continue;

            if (QGraphicsItem* gi = findItemByUid(uid))
            {
                _scene->removeItem(gi);
                removeListEntryBySceneItem(gi);
                delete gi;
            }
        }

        if (auto d = currentDocument())
        {
            d->isModified = true;
            updateFileListDisplay(d->filePath);
        }
    };

    // undo: восстановить по снапшотам
    auto undoFn = [this, payload]()
    {
        for (const ShapeBackup& b : std::as_const(payload->backups))
        {
            QGraphicsItem* created = recreateFromBackup(b);
            Q_UNUSED(created);
        }

        if (auto d = currentDocument())
        {
            d->isModified = true;
            updateFileListDisplay(d->filePath);
        }
    };

    if (doc->_undoStack)
        doc->_undoStack->push(new LambdaCommand(redoFn, undoFn, tr("Удаление фигур")));
}

void MainWindow::on_actAbout_triggered()
{
    // Диалоговое окно
    QDialog aboutDialog(this);
    aboutDialog.setWindowTitle(tr("О программе QLabelMe"));
    aboutDialog.resize(600, 500);
    aboutDialog.setMinimumSize(500, 400);

    // Вкладки
    QTabWidget *tabWidget = new QTabWidget(&aboutDialog);

    // Вкладка "О программе"
    QWidget *aboutTab = new QWidget();
    QVBoxLayout *aboutLayout = new QVBoxLayout(aboutTab);

    QLabel* titleLabel = new QLabel(tr("<b>QLabelMe<b>"));

    QLabel* versionLabel = new QLabel(
        tr("Версия: %1 (gitrev: %2)<br>Программа для разметки и аннотирования изображений.")
            .arg(VERSION_PROJECT)
            .arg(GIT_REVISION)
        );
    versionLabel->setWordWrap(true);

    QLabel* descriptionLabel = new QLabel(
        tr("Программа позволяет вручную создавать и редактировать аннотации с помощью графических примитивов. "
           "Используется для подготовки датасетов в задачах компьютерного зрения и машинного обучения.<br><br>"
           "<b>Обратная связь:</b><br>"
           "https://github.com/mariekarelina/qlabelme/issues<br><br>")
        );
    descriptionLabel->setWordWrap(true);
    descriptionLabel->setOpenExternalLinks(true);

    aboutLayout->addWidget(titleLabel);
    aboutLayout->addWidget(versionLabel);
    aboutLayout->addWidget(descriptionLabel);
    aboutLayout->addStretch();

    // Вкладка "Компоненты"
    QWidget *componentsTab = new QWidget();
    QVBoxLayout *componentsLayout = new QVBoxLayout(componentsTab);

    QTextEdit *componentsText = new QTextEdit();
    componentsText->setReadOnly(true);
    componentsText->setHtml(
        tr("<ul>"
           "<li>Qt %1 - кроссплатформенный фреймворк</li>"
           "</ul>")
            .arg(QT_VERSION_STR)
        );
    componentsLayout->addWidget(componentsText);

    // Вкладка "Авторы"
    QWidget *authorsTab = new QWidget();
    QVBoxLayout *authorsLayout = new QVBoxLayout(authorsTab);

    QTextEdit *authorsText = new QTextEdit();
    authorsText->setReadOnly(true);
    authorsText->setHtml(
        tr("<ul>"
           "<li>Карелина Мария - автор, разработчик</li>"
           "<li>Карелин Павел - консультант по коду и архитектуре программы</li>"
           "<li>Назаровский Александр - консультант по функциональной части программы</li>"
           "</ul>")
        );
    authorsLayout->addWidget(authorsText);

    // Добавляем вкладки
    tabWidget->addTab(aboutTab, tr("О программе"));
    tabWidget->addTab(componentsTab, tr("Компоненты"));
    tabWidget->addTab(authorsTab, tr("Авторы"));

    // Кнопка OK
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, &aboutDialog);
    connect(buttonBox, &QDialogButtonBox::accepted, &aboutDialog, &QDialog::accept);

    // Основной layout
    QVBoxLayout *mainLayout = new QVBoxLayout(&aboutDialog);
    mainLayout->addWidget(tabWidget);
    mainLayout->addWidget(buttonBox);

    // Устанавливаем иконку
    aboutDialog.setWindowIcon(QIcon(":/images/resources/qlabelme.png"));

    // Показываем диалог
    aboutDialog.exec();
}

void MainWindow::on_actSettingsApp_triggered()
{
    auto visBackup = _vis;
    bool wasApplied = false;

    Settings dlg(this);

    QVector<int> geom{ -1, -1, 520, 360 };
    config::base().getValue("windows.settings.geometry", geom);
    if (geom.size() == 4) {
        // Защита от слишком маленького размера
        if (geom[2] < 320)
            geom[2] = 320;
        if (geom[3] < 220)
            geom[3] = 220;

        if (geom[0] >= 0 && geom[1] >= 0)
        {
            dlg.setGeometry(geom[0], geom[1], geom[2], geom[3]);
        }
        else
        {
            dlg.resize(geom[2], geom[3]);
        }
    }

    Settings::Values init;
    init.lineWidth         = _vis.lineWidth;
    init.handleSize        = _vis.handleSize;
    init.numberFontPt      = _vis.numberFontPt;
    init.pointSize         = _vis.pointSize;

    init.nodeColor         = _vis.handleColor;           // «Цвет узла»
    init.nodeSelectedColor = _vis.selectedHandleColor;   // «Цвет выбранного узла»
    init.numberColor       = _vis.numberColor;           // «Цвет нумерации»
    init.numberBgColor     = _vis.numberBgColor;         // «Фон нумерации»
    init.rectLineColor     = _vis.rectangleLineColor;    // «Линии прямоугольника»
    init.circleLineColor   = _vis.circleLineColor;       // «Линии окружности»
    init.polylineLineColor = _vis.polylineLineColor;     // «Линии полилинии»
    init.lineLineColor     = _vis.lineLineColor;
    init.pointColor        = _vis.pointColor;

    init.closePolyline = _polylineCloseMode;    // Текущий режим замыкания
    init.finishLine    = _lineFinishMode;       // Текущий режим завершения рисования линии
    init.labelFontPt   = _vis.labelFontPt;      // Размер и шрифт
    init.labelFont     = _vis.labelFont;

    config::base().getValue("view.keep_image_scale_per_image", init.keepImageScale);
    config::base().getValue("ui.keep_menu_visibility", init.keepMenuBarVisibility);

    dlg.setValues(init);


    // connect(&dlg, &Settings::settingsApplied, this,
    //             [this](const Settings::Values& v)
    // {
    connect(&dlg, &Settings::settingsApplied, this,
            [this, &visBackup, &wasApplied](const Settings::Values& v)
    {
        wasApplied = true;
        // 1) Обновляем визуальные настройки из v
        _vis.lineWidth = v.lineWidth;
        _vis.handleSize = v.handleSize;
        _vis.numberFontPt = v.numberFontPt;
        _vis.pointSize = v.pointSize;

        _vis.handleColor = v.nodeColor;
        _vis.selectedHandleColor = v.nodeSelectedColor;
        _vis.numberColor = v.numberColor;
        _vis.numberBgColor = v.numberBgColor;
        _vis.rectangleLineColor = v.rectLineColor;
        _vis.circleLineColor = v.circleLineColor;
        _vis.polylineLineColor = v.polylineLineColor;
        _vis.lineLineColor = v.lineLineColor;
        _vis.pointColor = v.pointColor;
        _vis.pointOutlineWidth = v.pointOutlineWidth;
        _vis.labelFontPt = v.labelFontPt;
        _vis.labelFont = v.labelFont;
        applyLabelFontToUi();

        // 2) Сохраняем и применяем
        saveVisualStyle();
        applyStyle_AllDocuments();

        /*apply_LineWidth_ToScene(nullptr);
        apply_PointSize_ToScene(nullptr);
        apply_NumberSize_ToScene(nullptr)*/;

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
        visBackup = _vis;
    });

    const int rc = dlg.exec();
    // Геометрию восстановить/сохранить
    const QRect r = dlg.isMaximized() || dlg.isFullScreen()
                    ? dlg.normalGeometry()
                    : dlg.geometry();
    QVector<int> out { r.x(), r.y(), r.width(), r.height() };
    config::base().setValue("windows.settings.geometry", out);
    config::base().saveFile();

    if (rc == QDialog::Rejected)
    {
        _vis = visBackup;
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
    QList<QGraphicsItem*> selectedItems = _scene->selectedItems();
    for (QGraphicsItem* item : selectedItems)
    {
        if (auto* rectangle = dynamic_cast<qgraph::Rectangle*>(item))
        {
            rectangle->togglePointNumbers();
        }
        else if (auto* polyline = dynamic_cast<qgraph::Polyline*>(item))
        {
            polyline->togglePointNumbers();
        }
        else if (auto* line = dynamic_cast<qgraph::Line*>(item))
        {
            line->togglePointNumbers();
        }
    }
}

void MainWindow::on_actRotatePointsClockwise_triggered()
{
    QList<QGraphicsItem*> selectedItems = _scene->selectedItems();
    for (QGraphicsItem* item : selectedItems)
    {
        if (auto* rectangle = dynamic_cast<qgraph::Rectangle*>(item))
        {
            rectangle->rotatePointsClockwise();
            updateAllPointNumbers();
        }
        else if (auto* polyline = dynamic_cast<qgraph::Polyline*>(item))
        {
            polyline->rotatePointsClockwise();
            updateAllPointNumbers();
        }
        else if (auto* line = dynamic_cast<qgraph::Line*>(item))
        {
            line->rotatePointsClockwise();
            updateAllPointNumbers();
        }
    }
}

void MainWindow::on_actRotatePointsCounterClockwise_triggered()
{
    QList<QGraphicsItem*> selectedItems = _scene->selectedItems();
    for (QGraphicsItem* item : selectedItems)
    {
        if (auto* rectangle = dynamic_cast<qgraph::Rectangle*>(item))
        {
            rectangle->rotatePointsCounterClockwise();
            updateAllPointNumbers();
        }
        else if (auto* polyline = dynamic_cast<qgraph::Polyline*>(item))
        {
            polyline->rotatePointsCounterClockwise();
            updateAllPointNumbers();
        }
        else if (auto* line = dynamic_cast<qgraph::Line*>(item))
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
    if (!_scene)
        return;

    QList<QGraphicsItem*> selectedItems = _scene->selectedItems();
    for (QGraphicsItem* item : selectedItems)
    {
        if (auto* polyline = dynamic_cast<qgraph::Polyline*>(item))
        {
            polyline->toggleSelectionRect();
        }
        else if (auto* line = dynamic_cast<qgraph::Line*>(item))
        {
            line->toggleSelectionRect();
        }
        else if (auto* circle = dynamic_cast<qgraph::Circle*>(item))
        {
            circle->toggleSelectionRect();
        }
    }
}

void MainWindow::on_actUndo_triggered()
{
    if (!_undoGroup)
        return;

    if (_undoGroup->canUndo())
        _undoGroup->undo();
}

void MainWindow::on_actRedo_triggered()
{
    if (!_undoGroup)
        return;

    if (_undoGroup->canRedo())
        _undoGroup->redo();
}

void MainWindow::on_actResetAnnotation_triggered()
{
    auto doc = currentDocument();
    if (!doc)
        return;

    // Если разметки нет
    if (ui->polygonList->count() == 0)
        return;

    // Подтверждение
    QMessageBox msgBox(this);
    msgBox.setWindowTitle(tr("Сбросить разметку"));
    msgBox.setText(tr("Удалить всю разметку на текущем снимке?"));
    msgBox.setIcon(QMessageBox::Question);

    QPushButton* btnYes = msgBox.addButton(tr("Да"), QMessageBox::AcceptRole);
    QPushButton* btnNo  = msgBox.addButton(tr("Нет"), QMessageBox::RejectRole);
    msgBox.setDefaultButton(btnNo);

    msgBox.exec();

    if (msgBox.clickedButton() != btnYes)
        return;

    // Собираем все элементы из списка
    QList<QListWidgetItem*> allItems;
    allItems.reserve(ui->polygonList->count());
    for (int i = 0; i < ui->polygonList->count(); ++i)
    {
        if (auto* it = ui->polygonList->item(i))
            allItems.push_back(it);
    }

    // Снимки для восстановления
    const QVector<ShapeBackup> backups = collectBackupsForItems(allItems);

    struct Payload
    {
        QVector<ShapeBackup>  backups;
        QVector<qulonglong>   uids;
    };

    auto payload = std::make_shared<Payload>();
    payload->backups = backups;
    payload->uids.reserve(backups.size());

    for (const ShapeBackup& b : backups)
    {
        if (b.uid != 0)
            payload->uids.push_back(b.uid);
    }

    // redo: удалить все элементы по uid
    auto redoFn = [this, payload]()
    {
        for (qulonglong uid : std::as_const(payload->uids))
        {
            if (!uid)
                continue;

            if (QGraphicsItem* gi = findItemByUid(uid))
            {
                _scene->removeItem(gi);
                removeListEntryBySceneItem(gi);
                delete gi;
            }
        }

        if (auto d = currentDocument())
        {
            d->isModified = true;
            updateFileListDisplay(d->filePath);
        }
    };

    // undo: восстановить по снапшотам
    auto undoFn = [this, payload]()
    {
        for (const ShapeBackup& b : std::as_const(payload->backups))
        {
            QGraphicsItem* created = recreateFromBackup(b);
            Q_UNUSED(created);
        }

        if (auto d = currentDocument())
        {
            d->isModified = true;
            updateFileListDisplay(d->filePath);
        }
    };

    if (doc->_undoStack)
        doc->_undoStack->push(new LambdaCommand(redoFn, undoFn, tr("Сброс разметки")));
}


void MainWindow::on_actRestoreAnnotation_triggered()
{
    auto doc = currentDocument();
    if (!doc)
        return;

    const QString yamlPath = annotationPathFor(doc->filePath);

    if (!QFileInfo::exists(yamlPath))
    {
        QMessageBox::information(this,
                                 tr("Восстановить разметку"),
                                 tr("Файл разметки не найден:\n%1").arg(yamlPath));
        return;
    }

    // Восстановление откатит все несохраненные изменения
    const bool hasUnsaved = doc->isModified || (doc->_undoStack && !doc->_undoStack->isClean());

    if (hasUnsaved || ui->polygonList->count() > 0)
    {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle(tr("Восстановить разметку"));
        msgBox.setText(tr("Восстановить разметку из файла .yaml?\nВсе несохранённые изменения будут потеряны."));
        msgBox.setIcon(QMessageBox::Question);

        QPushButton* btnYes = msgBox.addButton(tr("Да"), QMessageBox::AcceptRole);
        QPushButton* btnNo  = msgBox.addButton(tr("Нет"), QMessageBox::RejectRole);
        msgBox.setDefaultButton(btnNo);

        msgBox.exec();

        if (msgBox.clickedButton() != btnYes)
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

void MainWindow::on_actUserGuide_triggered()
{

}

