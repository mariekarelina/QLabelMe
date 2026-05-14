#include "main_window.h"
#include "ui_main_window.h"
//#include "event_window.h"
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

#include <QApplication>
#include <QMessageBox>
#include <QFileDialog>
#include <QScrollBar>

// #include <QHostInfo>
// #include <QScreen>
// #include <QSlider>
// #include <QLabel>
// #include <QPointF>
// #include <QFileInfo>
// #include <QInputDialog>
// #include <QGraphicsItem>
// #include <QVBoxLayout>
// #include <QPixmap>
// #include <QFrame>
// #include <QMouseEvent>
// #include <QWheelEvent>
// #include <QEvent>
// #include <QKeyEvent>
// #include <unistd.h>
// #include <QPen>
// #include <QSplitter>
// #include <QListWidget>
// #include <QJsonDocument>
// #include <QJsonArray>
// #include <QJsonObject>
// #include <QClipboard>
// #include <QMimeData>
// #include <QGuiApplication>
// #include <QDoubleSpinBox>
// #include <QFormLayout>
// #include <QDialogButtonBox>
// #include <QAbstractButton>
// #include <QAction>
// #include <QMenu>
// #include <QGraphicsSimpleTextItem>
// #include <QGraphicsRectItem>
// #include <QAbstractItemView>
// #include <QMetaType>
// #include <QtGlobal>
// #include <QShortcut>
// #include <QKeySequence>
// #include <QPainterPathStroker>
// #include <QStyleFactory>

// #include <algorithm>
// #include <cmath>
// #include <limits>

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

// Простое сравнение геометрии снапшотов, чтобы не совершать no-op команды
static bool sameGeometry(const ShapeBackup& firstBackup, const ShapeBackup& secondBackup)
{
    if (firstBackup.kind != secondBackup.kind)
        return false;

    switch (firstBackup.kind)
    {
        case ShapeKind::Rectangle:
            return firstBackup.rect == secondBackup.rect
                && firstBackup.visible == secondBackup.visible
                && firstBackup.className == secondBackup.className;

        case ShapeKind::Circle:
            return firstBackup.circleCenter == secondBackup.circleCenter
                && firstBackup.circleRadius == secondBackup.circleRadius
                && firstBackup.visible == secondBackup.visible
                && firstBackup.className == secondBackup.className;

        case ShapeKind::Polyline:
            return firstBackup.points == secondBackup.points
                && firstBackup.closed == secondBackup.closed
                && firstBackup.visible == secondBackup.visible
                && firstBackup.className == secondBackup.className;

        case ShapeKind::Line:
            return firstBackup.points == secondBackup.points
                && firstBackup.numberingFromLast == secondBackup.numberingFromLast
                && firstBackup.visible == secondBackup.visible
                && firstBackup.className == secondBackup.className;

        case ShapeKind::Point:
            return firstBackup.pointCenter == secondBackup.pointCenter
                && firstBackup.visible == secondBackup.visible
                && firstBackup.className == secondBackup.className;
    }
    return false;
}

Document::Ptr Document::create(const QString& path)
{
    Ptr doc {new Document};
    doc->filePath = path;
    return doc;
}

bool Document::loadImage()
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

MainWindow::MainWindow(QWidget* parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    _scene(new QGraphicsScene(this))
    //_socket(new tcp::Socket)
{
    ui->setupUi(this);

    //ui->menuBar->setAttribute(Qt::WA_TransparentForMouseEvents, false);

    ui->graphView->setViewportUpdateMode(QGraphicsView::MinimalViewportUpdate);
    ui->graphView->setCacheMode(QGraphicsView::CacheBackground);

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
    void (*fixSelectionPalette)(QListWidget*) = [](QListWidget* listWidget){
        QPalette palette = listWidget->palette();
        const QColor activeHighlight = palette.color(QPalette::Active, QPalette::Highlight);
        const QColor activeHighlightedText = palette.color(QPalette::Active, QPalette::HighlightedText);

        palette.setColor(QPalette::Inactive, QPalette::Highlight,       activeHighlight);
        palette.setColor(QPalette::Inactive, QPalette::HighlightedText, activeHighlightedText);
        palette.setColor(QPalette::Disabled, QPalette::Highlight,       activeHighlight);
        palette.setColor(QPalette::Disabled, QPalette::HighlightedText, activeHighlightedText);

        listWidget->setPalette(palette);
        if (listWidget->viewport())
            listWidget->viewport()->setPalette(palette);
    };

    fixSelectionPalette(ui->polygonList);
    fixSelectionPalette(ui->fileList);

    qRegisterMetaType<QGraphicsItem*>("QGraphicsItem*");

    if (_scene)
        _scene->installEventFilter(this);

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
    connect(ui->polygonList, &QListWidget::customContextMenuRequested,
            this, &MainWindow::showPolygonListContextMenu);

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

    connect(ui->polygonList, &QListWidget::currentRowChanged,
            this, [this](int)
    {
        updateShapeListButtons();
    });

    connect(ui->polygonList, &QListWidget::itemSelectionChanged,
            this, &MainWindow::updateShapeListButtons);

    updateShapeListButtons();

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
    _rightPanel = ui->splitter->widget(1);
    // Сохраняем начальные размеры сплиттера
    _savedSplitterSizes = ui->splitter->sizes();
    applyLabelFontToUi();

    // сцена должна отдавать нам события
    _scene->installEventFilter(this);

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

    ensureGhostAllocated();

    //config::base().getValue("view.keep_image_scale", init.keepImageScale);

    // Приемник команд от фигур
    _scene->setProperty("classChangeReceiver", QVariant::fromValue(static_cast<QObject*>(this)));
    _scene->setProperty("shapeDeleteReceiver", QVariant::fromValue(static_cast<QObject*>(this)));
    _scene->setProperty("fillShapeWhenSelected", _vstyle.fillShapeWhenSelected);

    // Стек и действия
    // _undoStack = std::make_unique<QUndoStack>(this);
    // _actUndo = _undoStack->createUndoAction(this, u8"Отменить"));
    // _actRedo = _undoStack->createRedoAction(this, u8"Повторить"));

    // Группа стеков действий для разных документов
    _undoGroup = new QUndoGroup(this);

    // _actUndo = _undoGroup->createUndoAction(this, u8"Отменить"));
    // _actRedo = _undoGroup->createRedoAction(this, u8"Повторить"));

    // _actUndo->setShortcut(QKeySequence::Undo);
    // _actRedo->setShortcuts({QKeySequence::Redo, QKeySequence(Qt::CTRL | Qt::Key_Y)});

    // _actUndo->setShortcutContext(Qt::ApplicationShortcut);
    // _actRedo->setShortcutContext(Qt::ApplicationShortcut);

    // Фиксированные подписи без динамического хвоста
    // _actUndo = new QAction(u8"Отменить"), this);
    // _actRedo = new QAction(u8"Повторить"), this);

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

    // _actUndo->setToolTip(u8"Отменить"));
    // _actRedo->setToolTip(u8"Повторить"));

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

    // std::function<void()> undoBtn = qobject_cast<QToolButton*>(
    //     ui->toolBar->widgetForAction(_actUndo)
    // );
    // if (undoBtn)
    // {
    //     undoBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    // }

    // std::function<void()> redoBtn = qobject_cast<QToolButton*>(
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
    //     Document::Ptr doc = currentDocument();
    //     if (!doc)
    //         return;

    //     doc->isModified = true;
    //     updateFileListDisplay(doc->filePath);
    // });

    // Панель с QUndoView для визуального контроля
    // _undoView = new QUndoView(_undoStack.get(), this);
    // _undoView->setEmptyLabel(u8"<пусто>"));
    // _undoDock = new QDockWidget(u8"Стек действий (временный)"), this);
    // _undoDock->setObjectName("UndoDockDebug");
    // _undoDock->setWidget(_undoView);
    // addDockWidget(Qt::RightDockWidgetArea, _undoDock);

    _undoView = ui->undoView;
    //_undoView->setStack(_undoStack.get());
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

            for (QGraphicsItem* item : _scene->items())
            {
                const QString cls = item->data(0).toString();
                if (!cls.isEmpty())
                    applyClassColorToItem(item, cls);
            }
            _scene->update();
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
    // bool mbVisible = true;
    // ui->menuBar->setVisible(mbVisible);
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
        const Qt::KeyboardModifiers mods = mouseEvent->modifiers();
        const QPointF scenePos = graphView->mapToScene(mouseEvent->pos());
        // Удаление в eventFilter()
        // Alt + ЛКМ добавить узел на ребро
        if ((mods & Qt::AltModifier) && !(mods & Qt::ShiftModifier))
        {
            QGraphicsItem* item = pickItemByEdgeAt(graphView, mouseEvent->pos());

            if (qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(item))
            {
                ShapeBackup before = makeBackupFromItem(polyline);
                const qulonglong uid = before.uid;

                polyline->insertPoint(scenePos);

                ShapeBackup after = makeBackupFromItem(polyline);
                if (!sameGeometry(before, after))
                    pushModifyShapeCommand(uid, before, after, u8"Добавление узла");

                if (Document::Ptr doc = currentDocument(); doc && !doc->isModified)
                {
                    doc->isModified = true;
                    updateFileListDisplay(doc->filePath);
                }

                mouseEvent->accept();
                return;
            }

            if (qgraph::Line* line = dynamic_cast<qgraph::Line*>(item))
            {
                ShapeBackup before = makeBackupFromItem(line);
                const qulonglong uid = before.uid;

                line->insertPoint(scenePos);

                ShapeBackup after = makeBackupFromItem(line);
                if (!sameGeometry(before, after))
                    pushModifyShapeCommand(uid, before, after, u8"Добавление узла");

                if (Document::Ptr doc = currentDocument(); doc && !doc->isModified)
                {
                    doc->isModified = true;
                    updateFileListDisplay(doc->filePath);
                }

                mouseEvent->accept();
                return;
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
            _rulerStartPoint = graphView->mapToScene(mouseEvent->pos());
            _isDrawingRuler = true;
            updateModeLabel();

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

            QPen pen(_vstyle.rulerColor);
            pen.setWidthF(2.0);
            pen.setStyle(Qt::DotLine);
            pen.setCapStyle(Qt::RoundCap);
            pen.setJoinStyle(Qt::RoundJoin);
            pen.setCosmetic(true);

            _rulerLine = _scene->addLine(QLineF(_rulerStartPoint, _rulerStartPoint), pen);
            _rulerLine->setZValue(1000);

            _rulerText = _scene->addText(QStringLiteral("0"));
            _rulerText->setDefaultTextColor(_vstyle.rulerColor);
            // Масштабируем размер текста от размера изображения
            if (_videoRect)
            {
                const QSize imgSize = _videoRect->pixmap().size();
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

        if (selectionItem && selectionItem != _videoRect)
            selectionItem = findMovableAncestor(selectionItem);

        // Выделение через сцену, включая мультивыделение по Ctrl
        if (selectionItem && selectionItem != _videoRect)
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
                const QList<QGraphicsItem*> selectedNow = _scene->selectedItems();

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
            const QList<QGraphicsItem*> selectedNow = _scene->selectedItems();
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
                if (selectionItem && selectionItem != _videoRect)
                {
                    QGraphicsItem* movableItem = findMovableAncestor(selectionItem);

                    if (movableItem && (movableItem != _videoRect)
                        && movableItem->flags().testFlag(QGraphicsItem::ItemIsMovable))
                    {
                        _movingItem = movableItem;
                        //_moveBeforeSnap = makeBackupFromItem(_movingItem);
                        _moveHadChanges = false;
                        _moveInProgress = true;

                        const QPointF scenePos = graphView->mapToScene(mouseEvent->pos());
                        _moveGrabOffsetScene = scenePos - _movingItem->scenePos();
                        _moveSavedFlags = _movingItem->flags();

                        // Отключаем штатное Qt-перемещение, двигаем сами через _movingItem
                        _movingItem->setFlag(QGraphicsItem::ItemIsMovable, false);

                        _movingItems.clear();
                        //_moveGroupBefore.clear();
                        _moveGroupInitialPos.clear();
                        _moveIsGroup = false;
                        _movePressScenePos = scenePos;

                        const QList<QGraphicsItem*> selectedItems = _scene->selectedItems();
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
                                //_moveGroupBefore.append(makeBackupFromItem(root));
                                _moveGroupInitialPos.append(root->scenePos());
                            }

                            if (!_movingItems.contains(_movingItem))
                            {
                                _movingItems.append(_movingItem);
                                //_moveGroupBefore.append(makeBackupFromItem(_movingItem));
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
                _scene->addItem(_currCircle);
            }
            // Задаем позицию и начальный размер
            _currCircle->setRect(QRectF(_startPoint, QSizeF(1, 1)));
            _isDrawingCircle = true;

            const qreal crossSize = 6.0;
            if (!_currCircleCrossV)
            {
                _currCircleCrossV = _scene->addLine(0, 0, 0, 0);
                _currCircleCrossV->setFlag(QGraphicsItem::ItemIsMovable, false);
                _currCircleCrossV->setFlag(QGraphicsItem::ItemIsSelectable, false);
                _currCircleCrossV->setFlag(QGraphicsItem::ItemIsFocusable, false);
                _currCircleCrossV->setAcceptedMouseButtons(Qt::NoButton);

                _currCircleCrossV->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
                _currCircleCrossV->setZValue(1e9);
            }
            if (!_currCircleCrossH)
            {
                _currCircleCrossH = _scene->addLine(0, 0, 0, 0);
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
        updateModeLabel();
        const QPointF sceneP = graphView->mapToScene(mouseEvent->pos());

        _currPoint = new qgraph::Point(_scene);
        ensureUid(_currPoint);

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

                ShapeBackup backup = makeBackupFromItem(_currPoint);
                pushAdoptExistingShapeCommand(_currPoint, backup, u8"Добавление точки");
            }
        }

        _currPoint = nullptr;

        if (Document::Ptr doc = currentDocument())
        {
            doc->isModified = true;
            updateFileListDisplay(doc->filePath);
        }

        // Сразу завершаем (точка - единичный клик)
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
        //_isInDrawingMode = true;
        //setSceneItemsMovable(false);

        if (!_isDrawingRuler)
        {
            // Первая точка линейки
            _rulerStartPoint = graphView->mapToScene(mouseEvent->pos());

            _isDrawingRuler = true;
            updateModeLabel();

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

            QPen pen(_vstyle.rulerColor);
            pen.setWidthF(2.0);
            pen.setStyle(Qt::DotLine);
            pen.setCapStyle(Qt::RoundCap);
            pen.setJoinStyle(Qt::RoundJoin);
            pen.setCosmetic(true);

            _rulerLine = _scene->addLine(QLineF(_rulerStartPoint, _rulerStartPoint), pen);
            _rulerLine->setZValue(1000);

            // Текст с расстоянием
            _rulerText = _scene->addText(QStringLiteral("0"));
            _rulerText->setDefaultTextColor(_vstyle.rulerColor);
            // Масштабируем размер текста от размера изображения
            if (_videoRect)
            {
                const QSize imgSize = _videoRect->pixmap().size();
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
            _moveGroupBefore.clear();
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
    // if (_movingItem && (mouseEvent->buttons() & Qt::LeftButton))
    // {
    //     const QPointF scenePos = graphView->mapToScene(mouseEvent->pos());

    //     if (_moveIsGroup && !_movingItems.isEmpty())
    //     {
    //         // Сдвиг относительно положения курсора при захвате
    //         const QPointF shift = scenePos - _movePressScenePos;

    //         const int count = qMin(_movingItems.size(), _moveGroupInitialPos.size());
    //         for (int i = 0; i < count; ++i)
    //         {
    //             QGraphicsItem* item = _movingItems[i];
    //             if (!item)
    //                 continue;

    //             const QPointF& startPos = _moveGroupInitialPos[i];
    //             item->setPos(startPos + shift);
    //         }
    //     }
    //     else
    //     {
    //         _movingItem->setPos(scenePos - _moveGrabOffsetScene);
    //     }

    //     _moveHadChanges = true;
    // }
    // else if (_movingItem)
    // {
    //     // Перед сбросом возвращаем исходные флаги
    //     _movingItem->setFlags(_moveSavedFlags);
    //     // ЛКМ уже не зажата, но _movingItem остался - сбрасываем состояние
    //     _movingItem = nullptr;
    //     _movingItems.clear();
    //     _moveGroupBefore.clear();
    //     _moveGroupInitialPos.clear();
    //     _moveIsGroup = false;
    //     _moveHadChanges = false;
    //     _moveInProgress = false;
    // }
    if (_movingItem && (mouseEvent->buttons() & Qt::LeftButton))
    {
        const QPointF scenePos = graphView->mapToScene(mouseEvent->pos());

        if (!_movingItems.isEmpty() && !_moveGroupInitialPos.isEmpty())
        {
            QPointF shift;

            if (_moveIsGroup)
            {
                // Для группы все фигуры сдвигаются одинаково
                // относительно положения курсора в момент захвата.
                shift = scenePos - _movePressScenePos;
            }
            else
            {
                // Для одной фигуры учитываем место захвата внутри фигуры,
                // чтобы она не прыгала под курсор.
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
        // не делаем return обязательно, но можно:
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

            if (selectionItem && selectionItem != _videoRect)
                selectionItem = findMovableAncestor(selectionItem);

            if (selectionItem && selectionItem != _videoRect)
            {
                selectionItem->setSelected(!selectionItem->isSelected());
                mouseEvent->accept();
                return;
            }
        }

        mouseEvent->accept();
        return;
    }

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

        const bool keepLineLikeDrawing =
                _drawingPolyline || _drawingLine || _isDrawingPolyline || _isDrawingLine;

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
    /*if (mouseEvent->button() == Qt::LeftButton)
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
                                    ? u8"Перемещение фигуры"
                                    : u8"Перемещение фигур";

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
                                             u8"Перемещение фигуры");
                    }
                }
            }

            // if (_moveHadChanges)
            // {
            //     if (_moveIsGroup && !_movingItems.isEmpty())
            //     {
            //         for (QGraphicsItem* item : _movingItems)
            //         {
            //             if (qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(item))
            //                 polyline->updatePointNumbers();
            //             else if (qgraph::Line* line = dynamic_cast<qgraph::Line*>(item))
            //                 line->updatePointNumbers();
            //         }
            //     }
            //     else
            //     {
            //         if (qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(_movingItem))
            //             polyline->updatePointNumbers();
            //         else if (qgraph::Line* line = dynamic_cast<qgraph::Line*>(_movingItem))
            //             line->updatePointNumbers();
            //     }
            // }

            // _movingItem = nullptr;
            // _movingItems.clear();
            // _moveGroupBefore.clear();
            // _moveGroupInitialPos.clear();
            // _moveIsGroup = false;
            // _moveHadChanges = false;
            // _moveInProgress = false;
            // updateCoordinateList();
        }
    }*/

    if (mouseEvent->button() == Qt::LeftButton)
    {
        if (_movingItem)
        {
            _movingItem->setFlags(_moveSavedFlags);

            if (_moveHadChanges && !_movingItems.isEmpty())
            {
                const int count = qMin(_movingItems.size(), _moveGroupInitialPos.size());

                QList<QUuidEx> shapeIds;
                QPointF delta;
                bool hasMovedShapes = false;

                for (int i = 0; i < count; ++i)
                {
                    QGraphicsItem* item = _movingItems[i];

                    if (!item)
                        continue;

                    qgraph::Shape* shape = dynamic_cast<qgraph::Shape*>(item);

                    if (!shape)
                        continue;

                    const QUuidEx shapeId = shape->id();

                    if (shapeId.isNull())
                        continue;

                    if (!hasMovedShapes)
                    {
                        delta = item->scenePos() - _moveGroupInitialPos[i];

                        if (qFuzzyIsNull(delta.x()) && qFuzzyIsNull(delta.y()))
                            break;

                        hasMovedShapes = true;
                    }

                    shapeIds.append(shapeId);
                }

                if (hasMovedShapes && !shapeIds.isEmpty())
                {
                    const QString description = shapeIds.size() == 1
                                                ? u8"Перемещение фигуры"
                                                : u8"Перемещение фигур";

                    if (QUndoStack* stack = activeUndoStack())
                    {
                        stack->push(new undo::Move(_scene,
                                                   shapeIds,
                                                   description,
                                                   delta));
                    }

                    Document::Ptr doc = currentDocument();

                    if (doc)
                    {
                        doc->isModified = true;
                        updateFileListDisplay(doc->filePath);
                    }
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
        _scene->removeItem(_currRectangle);
        delete _currRectangle;
        _currRectangle = nullptr; // Сбрасываем ссылку, чтобы начать новый прямоугольник

        // Проверяем минимальный размер
        if (finalRect.width() < 1 || finalRect.height() < 1)
            return; // Не создаем объект, если размеры некорректны

        // Создаем объект класса Rectangle
        qgraph::Rectangle* rectangle = new qgraph::Rectangle(_scene);
        ensureUid(rectangle);
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

                // ShapeBackup b = makeBackupFromItem(rectangle);
                // pushAdoptExistingShapeCommand(rectangle, b, u8"Добавление прямоугольника");

            }
        }
        if (Document::Ptr doc = currentDocument())
        {
//------------------------------------------

            undo::RectangleData::Ptr rectData = undo::RectangleData::Ptr::create();
            rectData->id = rectangle->id();
            rectData->className = rectangle->data(0).toString();
            rectData->zLevel = rectangle->zValue();
            rectData->visible = rectangle->isVisible();
            rectData->rect = finalRect;
            rectData->shapeNumber = ensureShapeNumber(rectangle);

            undo::Create* undoCreate = new undo::Create(doc->scene, rectangle->id(),
                                                        u8"Добавление прямоугольника",
                                                        rectData);

            if (QUndoStack* stack = activeUndoStack())
            {
                stack->push(undoCreate);
            }

//-----------------------------------------


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
        if (circleRect.width() < 1 || circleRect.height() < 1)
            return; // Прерываем, если круг слишком маленький


        // Создаем объект класса Circle
        qgraph::Circle* circle = new qgraph::Circle(_scene, _startPoint);
        ensureUid(circle);
        // Устанавливаем радиус финального круга
        qreal finalRadius = circleRect.width() / 2;
        circle->setRealRadius(finalRadius);
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
                pushAdoptExistingShapeCommand(circle, b, u8"Добавление круга");

            }
        }
        if (Document::Ptr doc = currentDocument())
        {
            doc->isModified = true;
            updateFileListDisplay(doc->filePath);
        }
        raiseAllHandlesToTop();
    }
    else if (_drawingPolyline && _polyline && _polyline->isClosed())
    {
        _drawingPolyline = false;
        updateModeLabel();
        if (_polyline)
            _polyline->setFlag(QGraphicsItem::ItemIsMovable, true);

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
                    // _polyline->setData(0, selectedClass);
                    // applyClassColorToItem(_polyline, selectedClass);
                    // linkSceneItemToList(_polyline);

                    // ShapeBackup b = makeBackupFromItem(_polyline);
                    // pushAdoptExistingShapeCommand(_polyline, b, u8"Добавление полилинии"));
                    const qulonglong uid = ensureUid(_polyline);
                    const QString cls = selectedClass;

                    std::function<void()> redoFn = [this, uid, cls]()
                    {
                        qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(findItemByUid(uid));
                        if (!polyline)
                            return;

                        polyline->setClosed(true, true);
                        polyline->setFlag(QGraphicsItem::ItemIsMovable, true);

                        polyline->setData(0, cls);
                        applyClassColorToItem(polyline, cls);
                        linkSceneItemToList(polyline);

                        if (_polyline && ensureUid(_polyline) == uid)
                            _polyline = nullptr;

                        _drawingPolyline = false;
                        _resumeEditing = false;
                        _resumeUid = 0;
                        updateModeLabel();

                        if (Document::Ptr doc = currentDocument())
                        {
                            doc->isModified = true;
                            updateFileListDisplay(doc->filePath);
                        }
                    };

                    std::function<void()> undoFn = [this, uid]()
                    {
                        qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(findItemByUid(uid));
                        if (!polyline)
                            return;

                        polyline->setClosed(false, false);
                        polyline->setFlag(QGraphicsItem::ItemIsMovable, false);
                        _drawingPolyline = true;
                        _polyline = polyline;
                        updateModeLabel();

                        _resumeEditing = true;
                        _resumeUid = uid;

                        polyline->setSelected(true);
                        polyline->setFocus();

                        removeListEntryBySceneItem(polyline);

                        if (Document::Ptr doc = currentDocument())
                        {
                            doc->isModified = true;
                            updateFileListDisplay(doc->filePath);
                        }

                        raiseAllHandlesToTop();
                    };
                    if (QUndoStack* stack = activeUndoStack())
                        stack->push(new LambdaCommand(redoFn, undoFn, u8"Добавление полилинии"));
                    else
                        redoFn();
                }
            }
        }
        else
        {
            const QString cls = _polyline->data(0).toString();
            const qulonglong uid = ensureUid(_polyline);

            _drawingPolyline = false;
            updateModeLabel();

            std::function<void()> redoFn = [this, uid, cls]()
            {
                qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(findItemByUid(uid));
                if (!polyline)
                    return;

                polyline->setClosed(true, false);
                polyline->setFlag(QGraphicsItem::ItemIsMovable, true);

                if (!cls.isEmpty())
                {
                    polyline->setData(0, cls);
                    applyClassColorToItem(polyline, cls);
                }
                linkSceneItemToList(polyline);

                if (_polyline && ensureUid(_polyline) == uid)
                    _polyline = nullptr;

                _drawingPolyline = false;
                _resumeEditing = false;
                _resumeUid = 0;
                updateModeLabel();

                if (Document::Ptr doc = currentDocument())
                {
                    doc->isModified = true;
                    updateFileListDisplay(doc->filePath);
                }
                raiseAllHandlesToTop();
            };

            std::function<void()> undoFn = [this, uid]()
            {
                qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(findItemByUid(uid));
                if (!polyline)
                    return;

                polyline->setClosed(false, false);
                polyline->setFlag(QGraphicsItem::ItemIsMovable, false);

                _drawingPolyline = true;
                _polyline = polyline;
                updateModeLabel();

                _resumeEditing = true;
                _resumeUid = uid;

                polyline->setSelected(true);
                polyline->setFocus();

                removeListEntryBySceneItem(polyline);

                if (Document::Ptr doc = currentDocument())
                {
                    doc->isModified = true;
                    updateFileListDisplay(doc->filePath);
                }
                raiseAllHandlesToTop();
            };

            if (QUndoStack* stack = activeUndoStack())
                stack->push(new LambdaCommand(redoFn, undoFn, u8"Добавление полилинии"));
            else
                redoFn();
        }
        _polyline = nullptr;

        if (Document::Ptr doc = currentDocument())
        {
            doc->isModified = true;
            updateFileListDisplay(doc->filePath);
        }
        raiseAllHandlesToTop();
    }
    else if (_drawingLine && _line && _line->isClosed())
    {
        _drawingLine = false;
        updateModeLabel();
        if (_line)
                _line->setFlag(QGraphicsItem::ItemIsMovable, true);

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
                    // _line->setData(0, selectedClass);
                    // applyClassColorToItem(_line, selectedClass);
                    // linkSceneItemToList(_line);

                    // ShapeBackup b = makeBackupFromItem(_line);
                    // pushAdoptExistingShapeCommand(_line, b, u8"Добавление линии"));
                    const qulonglong uid = ensureUid(_line);
                    const QString cls = selectedClass;

                    _drawingLine = false;
                    updateModeLabel();

                    std::function<void()> redoFn = [this, uid, cls]()
                    {
                        qgraph::Line* line = dynamic_cast<qgraph::Line*>(findItemByUid(uid));
                        if (!line)
                            return;

                        line->setClosed(true, false);
                        line->setFlag(QGraphicsItem::ItemIsMovable, true);

                        line->setData(0, cls);
                        applyClassColorToItem(line, cls);
                        linkSceneItemToList(line);

                        if (_line && ensureUid(_line) == uid)
                            _line = nullptr;

                        _drawingLine = false;
                        _resumeEditing = false;
                        _resumeUid = 0;
                        updateModeLabel();

                        if (Document::Ptr doc = currentDocument())
                        {
                            doc->isModified = true;
                            updateFileListDisplay(doc->filePath);
                        }
                        raiseAllHandlesToTop();
                    };

                    std::function<void()> undoFn = [this, uid]()
                    {
                        qgraph::Line* line = dynamic_cast<qgraph::Line*>(findItemByUid(uid));
                        if (!line)
                            return;

                        line->setClosed(false, false);
                        line->setFlag(QGraphicsItem::ItemIsMovable, false);

                        _drawingLine = true;
                        _line = line;
                        updateModeLabel();

                        _resumeEditing = true;
                        _resumeUid = uid;

                        line->setSelected(true);
                        line->setFocus();

                        removeListEntryBySceneItem(line);

                        if (Document::Ptr doc = currentDocument())
                        {
                            doc->isModified = true;
                            updateFileListDisplay(doc->filePath);
                        }
                        raiseAllHandlesToTop();
                    };

                    if (QUndoStack* stack = activeUndoStack())
                        stack->push(new LambdaCommand(redoFn, undoFn, u8"Добавление линии"));
                    else
                        redoFn();
                }
            }
        }
        else
        {
            const qulonglong uid = ensureUid(_line);
            const QString cls = _line->data(0).toString();

            _drawingLine = false;
            updateModeLabel();

            std::function<void()> redoFn = [this, uid, cls]()
            {
                qgraph::Line* line = dynamic_cast<qgraph::Line*>(findItemByUid(uid));
                if (!line)
                    return;

                line->setClosed(true, false);
                line->setFlag(QGraphicsItem::ItemIsMovable, true);

                if (!cls.isEmpty())
                {
                    line->setData(0, cls);
                    applyClassColorToItem(line, cls);
                }
                linkSceneItemToList(line);

                if (_line && ensureUid(_line) == uid)
                    _line = nullptr;

                _drawingLine = false;
                _resumeEditing = false;
                _resumeUid = 0;
                updateModeLabel();

                if (Document::Ptr doc = currentDocument())
                {
                    doc->isModified = true;
                    updateFileListDisplay(doc->filePath);
                }
                raiseAllHandlesToTop();
            };

            std::function<void()> undoFn = [this, uid]()
            {
                qgraph::Line* line = dynamic_cast<qgraph::Line*>(findItemByUid(uid));
                if (!line)
                    return;

                line->setClosed(false, false);
                line->setFlag(QGraphicsItem::ItemIsMovable, false);

                _drawingLine = true;
                _line = line;
                updateModeLabel();

                _resumeEditing = true;
                _resumeUid = uid;

                line->setSelected(true);
                line->setFocus();

                removeListEntryBySceneItem(line);

                if (Document::Ptr doc = currentDocument())
                {
                    doc->isModified = true;
                    updateFileListDisplay(doc->filePath);
                }
                raiseAllHandlesToTop();
            };

            if (QUndoStack* stack = activeUndoStack())
                stack->push(new LambdaCommand(redoFn, undoFn, u8"Добавление линии"));
            else
                redoFn();
        }

        _line = nullptr;

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

void MainWindow::changeClassByUid(qulonglong uid)
{
    if (!_scene || uid == 0)
        return;

    QGraphicsItem* target = nullptr;

    for (QGraphicsItem* item : _scene->items())
    {
        if ((!item)
            || (item == _videoRect)
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

    if (qgraph::DragCircle* handle = dynamic_cast<qgraph::DragCircle*>(item))
        item = handle->parentItem();

    if (!item || item == _videoRect)
        return;

    item = findMovableAncestor(item);
    if (!item)
        return;

    QList<QGraphicsItem*> targets;

    // Сначала пытаемся собрать группу из выделения в правом списке,
    // так как команда вызывается из контекстного меню polygonList
    QList<QListWidgetItem*> selectedListItems;
    if (ui && ui->polygonList)
        selectedListItems = ui->polygonList->selectedItems();

    bool clickedItemIsInListSelection = false;

    for (QListWidgetItem* listItem : selectedListItems)
    {
        if (!listItem)
            continue;

        QGraphicsItem* selectedSceneItem = listItem->data(Qt::UserRole).value<QGraphicsItem*>();
        if (!selectedSceneItem)
            continue;

        if (qgraph::DragCircle* handle = dynamic_cast<qgraph::DragCircle*>(selectedSceneItem))
            selectedSceneItem = handle->parentItem();

        selectedSceneItem = findMovableAncestor(selectedSceneItem);

        if (!selectedSceneItem || selectedSceneItem == _videoRect)
            continue;

        if (selectedSceneItem == item)
            clickedItemIsInListSelection = true;
    }

    if (clickedItemIsInListSelection)
    {
        for (QListWidgetItem* listItem : selectedListItems)
        {
            if (!listItem)
                continue;

            QGraphicsItem* selectedSceneItem = listItem->data(Qt::UserRole).value<QGraphicsItem*>();
            if (!selectedSceneItem)
                continue;

            if (qgraph::DragCircle* handle = dynamic_cast<qgraph::DragCircle*>(selectedSceneItem))
                selectedSceneItem = handle->parentItem();

            selectedSceneItem = findMovableAncestor(selectedSceneItem);

            if (!selectedSceneItem || selectedSceneItem == _videoRect)
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

    if (QUndoStack* st = activeUndoStack())
        st->beginMacro(description);

    for (QGraphicsItem* target : targets)
    {
        if (!target)
            continue;

        ShapeBackup before = makeBackupFromItem(target);
        const qulonglong uid = before.uid;

        target->setVisible(!hideMode);

        if (hideMode)
            target->setSelected(false);

        ShapeBackup after = makeBackupFromItem(target);
        if (!sameGeometry(before, after))
            pushModifyShapeCommand(uid, before, after, description);
    }

    if (QUndoStack* st = activeUndoStack())
        st->endMacro();

    updatePolygonListTexts();

    if (Document::Ptr doc = currentDocument())
    {
        doc->isModified = true;
        updateFileListDisplay(doc->filePath);
    }
    updateCoordinateList();

    if (_scene)
        _scene->update();
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
                        ShapeBackup before = makeBackupFromItem(polyline);
                        const qulonglong uid = before.uid;

                        polyline->handlePointDeletion(handle);

                        ShapeBackup after = makeBackupFromItem(polyline);
                        if (!sameGeometry(before, after))
                            pushModifyShapeCommand(uid, before, after, u8"Удаление узла");

                        if (Document::Ptr doc = currentDocument(); doc && !doc->isModified)
                        {
                            doc->isModified = true;
                            updateFileListDisplay(doc->filePath);
                        }

                        event->accept();
                        return true;
                    }

                    if (qgraph::Line* line = dynamic_cast<qgraph::Line*>(handle->parentItem()))
                    {
                        ShapeBackup before = makeBackupFromItem(line);
                        const qulonglong uid = before.uid;

                        line->handlePointDeletion(handle);

                        ShapeBackup after = makeBackupFromItem(line);
                        if (!sameGeometry(before, after))
                            pushModifyShapeCommand(uid, before, after, u8"Удаление узла");

                        if (Document::Ptr doc = currentDocument(); doc && !doc->isModified)
                        {
                            doc->isModified = true;
                            updateFileListDisplay(doc->filePath);
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

            if (_drawingPolyline && _polyline &&
                qgraph::Polyline::globalCloseMode() == qgraph::Polyline::CloseMode::SingleClickOnFirstPoint)
            {
                const QVector<qgraph::DragCircle*>& circles = _polyline->circles();
                if (!circles.isEmpty())
                {
                    const QPointF p0 = circles.first()->scenePos();
                    const qreal kHit = _drawHandleCommitRadius;
                    if (QLineF(sp, p0).length() <= kHit)
                    {
                        //_drawingPolyline = false;
                        //_polyline->closePolyline();
                        if (_polyline && !_polyline->isClosed())
                        {
                            _polyline->closePolyline();
                        }
                        event->accept();
                        return true;

                        if (!_resumeEditing)
                        {
                            SelectClass dialog(_projectClasses, this);
                            if (dialog.exec() == QDialog::Accepted)
                            {
                                const QString selectedClass = dialog.selectedClass();
                                if (!selectedClass.isEmpty())
                                {
                                    // _polyline->setData(0, selectedClass);
                                    // applyClassColorToItem(_polyline, selectedClass);
                                    // linkSceneItemToList(_polyline);

                                    // ShapeBackup b = makeBackupFromItem(_polyline);
                                    // pushAdoptExistingShapeCommand(_polyline, b, u8"Добавление полилинии"));
                                    const qulonglong uid = ensureUid(_polyline);
                                    const QString cls = selectedClass;

                                    std::function<void()> redoFn = [this, uid, cls]()
                                    {
                                        qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(findItemByUid(uid));
                                        if (!polyline)
                                            return;

                                        polyline->setClosed(true, true);
                                        polyline->setFlag(QGraphicsItem::ItemIsMovable, true);

                                        polyline->setData(0, cls);
                                        applyClassColorToItem(polyline, cls);
                                        linkSceneItemToList(polyline);

                                        if (_polyline && ensureUid(_polyline) == uid)
                                            _polyline = nullptr;

                                        _drawingPolyline = false;
                                        updateModeLabel();
                                        _resumeEditing = false;
                                        _resumeUid = 0;

                                        if (Document::Ptr doc = currentDocument())
                                        {
                                            doc->isModified = true;
                                            updateFileListDisplay(doc->filePath);
                                        }
                                    };

                                    std::function<void()> undoFn = [this, uid]()
                                    {
                                        qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(findItemByUid(uid));
                                        if (!polyline) return;

                                        polyline->setClosed(false, false);
                                        polyline->setFlag(QGraphicsItem::ItemIsMovable, false);

                                        _drawingPolyline = true;
                                        _polyline = polyline;
                                        updateModeLabel();

                                        _resumeEditing = true;
                                        _resumeUid = uid;

                                        polyline->setSelected(true);
                                        polyline->setFocus();

                                        removeListEntryBySceneItem(polyline);

                                        if (Document::Ptr doc = currentDocument())
                                        {
                                            doc->isModified = true;
                                            updateFileListDisplay(doc->filePath);
                                        }

                                        raiseAllHandlesToTop();
                                    };
                                    if (QUndoStack* stack = activeUndoStack())
                                        stack->push(new LambdaCommand(redoFn, undoFn, u8"Добавление полилинии"));
                                    else
                                        redoFn();

                                }
                            }
                        }
                        else
                        {
                            const QString cls = _polyline->data(0).toString();
                            const qulonglong uid = ensureUid(_polyline);

                            _drawingPolyline = false;
                            updateModeLabel();

                            std::function<void()> redoFn = [this, uid, cls]()
                            {
                                qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(findItemByUid(uid));
                                if (!polyline)
                                    return;

                                polyline->setClosed(true, false);
                                polyline->setFlag(QGraphicsItem::ItemIsMovable, true);

                                if (!cls.isEmpty())
                                {
                                    polyline->setData(0, cls);
                                    applyClassColorToItem(polyline, cls);
                                }
                                linkSceneItemToList(polyline);

                                if (_polyline && ensureUid(_polyline) == uid)
                                    _polyline = nullptr;

                                _drawingPolyline = false;
                                updateModeLabel();
                                _resumeEditing = false;
                                _resumeUid = 0;

                                if (Document::Ptr doc = currentDocument())
                                {
                                    doc->isModified = true;
                                    updateFileListDisplay(doc->filePath);
                                }
                                raiseAllHandlesToTop();
                            };

                            std::function<void()> undoFn = [this, uid]()
                            {
                                qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(findItemByUid(uid));
                                if (!polyline)
                                    return;

                                polyline->setClosed(false, false);
                                polyline->setFlag(QGraphicsItem::ItemIsMovable, false);

                                _drawingPolyline = true;
                                _polyline = polyline;
                                updateModeLabel();

                                _resumeEditing = true;
                                _resumeUid = uid;

                                polyline->setSelected(true);
                                polyline->setFocus();

                                removeListEntryBySceneItem(polyline);

                                if (Document::Ptr doc = currentDocument())
                                {
                                    doc->isModified = true;
                                    updateFileListDisplay(doc->filePath);
                                }
                                raiseAllHandlesToTop();
                            };

                            if (QUndoStack* stack = activeUndoStack())
                                stack->push(new LambdaCommand(redoFn, undoFn, u8"Добавление полилинии"));
                            else
                                redoFn();
                        }

                        _polyline = nullptr;

                        if (Document::Ptr doc = currentDocument())
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

            const bool drawingLineLike =
                    _drawingPolyline || _drawingLine || _isDrawingPolyline || _isDrawingLine;

            // Во время рисования line/polyline не даем ближайшему узлу перехватывать клик
            // Захватываем существующий узел только при очень точном попадании
            if (drawingLineLike)
            {
                if (qgraph::DragCircle* preciseHandle = pickHandleAt(sp, _drawHandleCommitRadius))
                {
                    if (preciseHandle->scene() && _scene && _scene->items().contains(preciseHandle))
                    {
                        startHandleDrag(preciseHandle, sp);
                        return true;
                    }
                }

                break; // иначе пусть клик дойдет до handleLineLmbClick / handlePolylineLmbClick
            }

            if (qgraph::DragCircle* target = pickHiddenHandle(sp, topIsHandle))
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

            const QPoint  viewPos = ce->pos();
            const QPointF scenePos = ui->graphView->mapToScene(viewPos);
            QGraphicsItem* item = ui->graphView->itemAt(viewPos);
            if (!item || item == _videoRect)
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
                            ShapeBackup before = makeBackupFromItem(polyline);
                            qulonglong uid = before.uid;

                            polyline->handlePointDeletion(circle);

                            ShapeBackup after = makeBackupFromItem(polyline);
                            if (!sameGeometry(before, after))
                                pushModifyShapeCommand(uid, before, after, u8"Удаление узла");

                            if (Document::Ptr doc = currentDocument(); doc && !doc->isModified)
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
                            updateModeLabel();

                            _polyline->setModificationCallback([this]()
                            {
                                if (_drawingPolyline && _polyline && _polyline->isClosed())
                                {
                                    _drawingPolyline = false;
                                    updateModeLabel();

                                    _polyline->updatePointNumbers();
                                    apply_LineWidth_ToItem(_polyline);
                                    apply_PointSize_ToItem(_polyline);
                                    apply_NumberSize_ToItem(_polyline);

                                    ShapeBackup after = makeBackupFromItem(_polyline);
                                    if (!sameGeometry(_resumeBefore, after))
                                        pushModifyShapeCommand(_resumeUid, _resumeBefore, after, u8"Продолжение полилинии");

                                    _polyline = nullptr;
                                    _resumeEditing = false;
                                    _resumeUid = 0;

                                    if (Document::Ptr doc = currentDocument(); doc && !doc->isModified)
                                    {
                                        doc->isModified = true;
                                        updateFileListDisplay(doc->filePath);
                                    }

                                    raiseAllHandlesToTop();
                                    setSceneItemsMovable(true);
                                }
                            });
                        }
                        else if (chosen == actRecalc)
                        {
                            ShapeBackup before = makeBackupFromItem(polyline);
                            qulonglong uid = before.uid;

                            const  QVector<qgraph::DragCircle*>& circles = polyline->circles();
                            const int idx = circles.indexOf(circle);
                            if (idx < 0)
                            {
                                ce->accept();
                                return true;
                            }

                            for (int i = 0; i < idx; ++i)
                                polyline->rotatePointsClockwise();

                            ShapeBackup after = makeBackupFromItem(polyline);
                            if (!sameGeometry(before, after))
                                pushModifyShapeCommand(uid, before, after, u8"Пересчет нумерации");

                            if (Document::Ptr doc = currentDocument(); doc && !doc->isModified)
                            {
                                doc->isModified = true;
                                updateFileListDisplay(doc->filePath);
                            }
                        }
                        else if (chosen == actClockwise)
                        {
                            ShapeBackup before = makeBackupFromItem(polyline);
                            qulonglong uid = before.uid;

                            polyline->renumberFromHandleClockwise(circle);

                            ShapeBackup after = makeBackupFromItem(polyline);
                            if (!sameGeometry(before, after))
                                pushModifyShapeCommand(uid, before, after, u8"Нумерация по часовой стрелке");

                            if (Document::Ptr doc = currentDocument(); doc && !doc->isModified)
                            {
                                doc->isModified = true;
                                updateFileListDisplay(doc->filePath);
                            }
                        }
                        else if (chosen == actCounterClockwise)
                        {
                            ShapeBackup before = makeBackupFromItem(polyline);
                            qulonglong uid = before.uid;

                            polyline->renumberFromHandleCounterClockwise(circle);

                            ShapeBackup after = makeBackupFromItem(polyline);
                            if (!sameGeometry(before, after))
                                pushModifyShapeCommand(uid, before, after, u8"Нумерация против часовой стрелки");

                            if (Document::Ptr doc = currentDocument(); doc && !doc->isModified)
                            {
                                doc->isModified = true;
                                updateFileListDisplay(doc->filePath);
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
                            ShapeBackup before = makeBackupFromItem(line);
                            qulonglong uid = before.uid;

                            line->handlePointDeletion(circle);

                            ShapeBackup after = makeBackupFromItem(line);
                            if (!sameGeometry(before, after))
                                pushModifyShapeCommand(uid, before, after, u8"Удаление узла");

                            if (Document::Ptr doc = currentDocument(); doc && !doc->isModified)
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
                            updateModeLabel();

                            setSceneItemsMovable(false);

                            _line->setModificationCallback([this]()
                            {
                                if (_drawingLine && _line && _line->isClosed())
                                {
                                    _drawingLine = false;
                                    updateModeLabel();

                                    _line->updatePointNumbers();
                                    apply_LineWidth_ToItem(_line);
                                    apply_PointSize_ToItem(_line);
                                    apply_NumberSize_ToItem(_line);

                                    ShapeBackup after = makeBackupFromItem(_line);
                                    if (!sameGeometry(_resumeBefore, after))
                                        pushModifyShapeCommand(_resumeUid, _resumeBefore, after, u8"Продолжение линии");

                                    _line = nullptr;
                                    _resumeEditing = false;
                                    _resumeUid = 0;

                                    if (Document::Ptr doc = currentDocument(); doc && !doc->isModified)
                                    {
                                        doc->isModified = true;
                                        updateFileListDisplay(doc->filePath);
                                    }

                                    raiseAllHandlesToTop();
                                    setSceneItemsMovable(true);
                                }
                            });
                        }
                        else if (actRecalc && chosen == actRecalc)
                        {
                            ShapeBackup before = makeBackupFromItem(line);
                            qulonglong uid = before.uid;

                            const bool fromLast = (idx == circles.size() - 1);
                            line->setNumberingFromLast(fromLast);

                            ShapeBackup after = makeBackupFromItem(line);
                            if (!sameGeometry(before, after))
                                pushModifyShapeCommand(uid, before, after, u8"Пересчет нумерации");

                            if (Document::Ptr doc = currentDocument(); doc && !doc->isModified)
                            {
                                doc->isModified = true;
                                updateFileListDisplay(doc->filePath);
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
                            ShapeBackup before = makeBackupFromItem(rect);
                            qulonglong uid = before.uid;

                            rect->recalcNumberingFromHandle(circle);

                            ShapeBackup after = makeBackupFromItem(rect);
                            if (!sameGeometry(before, after))
                                pushModifyShapeCommand(uid, before, after, u8"Пересчет нумерации");

                            if (Document::Ptr doc = currentDocument(); doc && !doc->isModified)
                            {
                                doc->isModified = true;
                                updateFileListDisplay(doc->filePath);
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
                    ShapeBackup before = makeBackupFromItem(polyline);
                    qulonglong uid = before.uid;

                    polyline->insertPoint(scenePos);

                    ShapeBackup after = makeBackupFromItem(polyline);
                    if (!sameGeometry(before, after))
                        pushModifyShapeCommand(uid, before, after, u8"Добавление узла");

                    if (Document::Ptr doc = currentDocument(); doc && !doc->isModified)
                    {
                        doc->isModified = true;
                        updateFileListDisplay(doc->filePath);
                    }
                }
                else if (chosen == actToLine)
                {
                    ShapeBackup before = makeBackupFromItem(polyline);
                    const int n = before.points.size();
                    if (n < 2)
                    {
                        ce->accept();
                        return true;
                    }

                    ShapeBackup after = before;
                    after.kind = ShapeKind::Line;
                    after.closed = true;
                    after.numberingFromLast = false;

                    // Если полилиния замкнута, разрываем именно то ребро, по которому был ПКМ
                    if (polyline->isClosed() && n >= 3)
                    {
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

                        int bestIdx = 0;
                        double bestD2 = std::numeric_limits<double>::infinity();

                        // Ищем ближайший сегмент
                        for (int i = 0; i < n; ++i)
                        {
                            const QPointF& firstPt = before.points[i];
                            const QPointF& secondPt = before.points[(i + 1) % n];
                            const double d2 = dist2PointToSegment(scenePos, firstPt, secondPt);
                            if (d2 < bestD2)
                            {
                                bestD2 = d2;
                                bestIdx = i;
                            }
                        }
                        QVector<QPointF> reordered;
                        reordered.reserve(n);
                        const int start = (bestIdx + 1) % n; // Новая первая точка
                        for (int k = 0; k < n; ++k)
                            reordered.push_back(before.points[(start + k) % n]);

                        after.points = reordered;
                    }

                    pushReplaceShapeCommand(before.uid, before, after, u8"Полилиния -> линия");
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
                    ShapeBackup before = makeBackupFromItem(line);
                    qulonglong uid = before.uid;

                    line->insertPoint(scenePos);

                    ShapeBackup after = makeBackupFromItem(line);
                    if (!sameGeometry(before, after))
                        pushModifyShapeCommand(uid, before, after, u8"Добавление узла");

                    if (Document::Ptr doc = currentDocument(); doc && !doc->isModified)
                    {
                        doc->isModified = true;
                        updateFileListDisplay(doc->filePath);
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
                    ShapeBackup before = makeBackupFromItem(line);
                    if (before.points.size() < 2)
                    {
                        ce->accept();
                        return true;
                    }

                    ShapeBackup after = before;
                    after.kind = ShapeKind::Polyline;
                    after.closed = true;
                    after.numberingFromLast = false;

                    pushReplaceShapeCommand(before.uid, before, after, u8"Линия -> полилиния");
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

void MainWindow::keyPressEvent(QKeyEvent* event)
{
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
            ui->graphView->setTransform(base);    // Вернуть базу
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
            if (_scene)
                _scene->update();

            event->accept();
            return;
        }

        if (_drawingLine && _line)
        {
            _line->togglePointNumbers();
            _line->updatePointNumbers();
            if (_scene)
                _scene->update();

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

    // Получаем домашнюю директорию пользователя как начальную для диалога
    //QString initialDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);

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

    if (_m_zoom <= 0.000001)
        _m_zoom = 1.0;

    if (Document::Ptr doc = currentDocument())
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
    _m_zoom = ui->graphView->transform().m11();
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

            // Если есть изменения в сцене, сохраняем их в документе
            if (prevDoc->scene != _scene)
            {
                // Переносим все элементы (кроме videoRect) в документ предыдущего изображения
                QList<QGraphicsItem*> items = _scene->items();
                for (QGraphicsItem* item : items)
                {
                    if (!item)
                        continue;

                    if (item == _videoRect)
                        continue;

                    // Переносим только корневые элементы.
                    // Дочерние (ручки, крестик круга, рамки и т.п.) переедут вместе с родителем автоматически.
                    if (item->parentItem() != nullptr)
                        continue;

                    if (QGraphicsScene* sc = item->scene())
                        sc->removeItem(item);

                    prevDoc->scene->addItem(item);
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
    // Приемник команд от фигур на каждую сцену
    if (_scene)
    {
        _scene->setProperty("classChangeReceiver", QVariant::fromValue(static_cast<QObject*>(this)));
        _scene->setProperty("shapeDeleteReceiver", QVariant::fromValue(static_cast<QObject*>(this)));
        _scene->setProperty("fillShapeWhenSelected", _vstyle.fillShapeWhenSelected);
    }

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
    //updateAllPointNumbers();
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

    // Во время перетаскивания фигур не пересчитываем координаты
    if (_moveInProgress)
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

    if (_syncingSelection)
        return;

    _syncingSelection = true;

    QList<QListWidgetItem*> selected = ui->polygonList->selectedItems();

    {
        QSignalBlocker blockScene(_scene);

        for (QGraphicsItem* item : _scene->items())
        {
            if (!item)
                continue;

            if (item == _videoRect
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

        for (QListWidgetItem* li : selected)
        {
            if (!li)
                continue;

            if (QGraphicsItem* scItem = li->data(Qt::UserRole).value<QGraphicsItem*>())
                scItem->setSelected(true);
        }
    }

    updateCoordinateList();

    if (!selected.isEmpty())
    {
        if (QGraphicsItem* scItem = selected.first()->data(Qt::UserRole).value<QGraphicsItem*>())
            ui->graphView->ensureVisible(scItem);
    }

    raiseSelectedShapesTemporarily();
    updateShapeListButtons();

    _syncingSelection = false;
}

void MainWindow::selectAllShapes()
{
    if (!_scene || !ui || !ui->polygonList)
        return;

    _syncingSelection = true;

    {
        QSignalBlocker blockScene(_scene);
        QSignalBlocker blockList(ui->polygonList);

        ui->polygonList->clearSelection();

        for (QGraphicsItem* item : _scene->items())
        {
            if (!item)
                continue;

            if (item == _videoRect
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

        for (int i = 0; i < ui->polygonList->count(); ++i)
        {
            QListWidgetItem* li = ui->polygonList->item(i);
            if (li)
                li->setSelected(true);
        }
    }

    _syncingSelection = false;

    updateCoordinateList();
    raiseSelectedShapesTemporarily();
    _scene->update();
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

    updateModeLabel();
}

void MainWindow::on_actClosePolyline_triggered()
{
    if (_drawingPolyline && _polyline)
    {
        if (_polyline && !_polyline->isClosed())
            _polyline->closePolyline();
    }
    // Завершение рисования линии, если мы ее сейчас рисуем
    if (_drawingLine && _line)
    {
        _drawingLine = false;
        _line->closeLine();
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
                    // _line->setData(0, selectedClass);
                    // applyClassColorToItem(_line, selectedClass);
                    // linkSceneItemToList(_line);

                    // ShapeBackup b = makeBackupFromItem(_line);
                    // pushAdoptExistingShapeCommand(_line, b, u8"Добавление линии"));
                    const qulonglong uid = ensureUid(_line);
                    const QString cls = selectedClass;

                    _drawingLine = false;
                    updateModeLabel();

                    std::function<void()> redoFn = [this, uid, cls]()
                    {
                        qgraph::Line* line = dynamic_cast<qgraph::Line*>(findItemByUid(uid));
                        if (!line)
                            return;

                        line->setClosed(true, false);
                        line->setFlag(QGraphicsItem::ItemIsMovable, true);

                        line->setData(0, cls);
                        applyClassColorToItem(line, cls);
                        linkSceneItemToList(line);

                        if (_line && ensureUid(_line) == uid)
                            _line = nullptr;

                        _drawingLine = false;
                        _resumeEditing = false;
                        _resumeUid = 0;
                        updateModeLabel();

                        if (Document::Ptr doc = currentDocument())
                        {
                            doc->isModified = true;
                            updateFileListDisplay(doc->filePath);
                        }
                        raiseAllHandlesToTop();
                    };

                    std::function<void()> undoFn = [this, uid]()
                    {
                        qgraph::Line* line = dynamic_cast<qgraph::Line*>(findItemByUid(uid));
                        if (!line)
                            return;

                        line->setClosed(false, false);
                        line->setFlag(QGraphicsItem::ItemIsMovable, false);

                        _drawingLine = true;
                        _line = line;
                        _resumeEditing = true;
                        _resumeUid = uid;
                        updateModeLabel();

                        line->setSelected(true);
                        line->setFocus();

                        removeListEntryBySceneItem(line);

                        if (Document::Ptr doc = currentDocument())
                        {
                            doc->isModified = true;
                            updateFileListDisplay(doc->filePath);
                        }
                        raiseAllHandlesToTop();
                    };

                    if (QUndoStack* stack = activeUndoStack())
                        stack->push(new LambdaCommand(redoFn, undoFn, u8"Добавление линии"));
                    else
                        redoFn();
                }
            }
        }
        else
        {
            const qulonglong uid = ensureUid(_line);
            const QString cls = _line->data(0).toString();

            _drawingLine = false;
            updateModeLabel();

            std::function<void()> redoFn = [this, uid, cls]()
            {
                qgraph::Line* line = dynamic_cast<qgraph::Line*>(findItemByUid(uid));
                if (!line) return;

                line->setClosed(true, false);
                line->setFlag(QGraphicsItem::ItemIsMovable, true);

                if (!cls.isEmpty())
                {
                    line->setData(0, cls);
                    applyClassColorToItem(line, cls);
                }
                linkSceneItemToList(line);

                if (_line && ensureUid(_line) == uid)
                    _line = nullptr;

                _drawingLine = false;
                _resumeEditing = false;
                _resumeUid = 0;
                updateModeLabel();

                if (Document::Ptr doc = currentDocument())
                {
                    doc->isModified = true;
                    updateFileListDisplay(doc->filePath);
                }
                raiseAllHandlesToTop();
            };

            std::function<void()> undoFn = [this, uid]()
            {
                qgraph::Line* line = dynamic_cast<qgraph::Line*>(findItemByUid(uid));
                if (!line) return;

                line->setClosed(false, false);
                line->setFlag(QGraphicsItem::ItemIsMovable, false);

                _drawingLine = true;
                _line = line;

                _resumeEditing = true;
                _resumeUid = uid;
                updateModeLabel();

                line->setSelected(true);
                line->setFocus();

                removeListEntryBySceneItem(line);

                if (Document::Ptr doc = currentDocument())
                {
                    doc->isModified = true;
                    updateFileListDisplay(doc->filePath);
                }
                raiseAllHandlesToTop();
            };

            if (QUndoStack* stack = activeUndoStack())
                stack->push(new LambdaCommand(redoFn, undoFn, u8"Добавление линии"));
            else
                redoFn();
        }
        _line = nullptr;

        if (Document::Ptr doc = currentDocument())
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
    _imageDataMap.clear();

    ui->fileList->clear();
    ui->polygonList->clear();
    ui->coordinateList->clear();

    if (_undoView)
        _undoView->setStack(nullptr);

    _scene = nullptr;
    _videoRect = nullptr;

    updateWindowTitle();
    updateFolderPathDisplay();

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
    // Пропускаем временные элементы и само изображение
    if (item == _videoRect || item == _tempRectItem ||
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

    // Ищем соответствующий элемент в списке полигонов
    for (int i = 0; i < ui->polygonList->count(); ++i)
    {
        QListWidgetItem* listItem = ui->polygonList->item(i);
        QVariant itemData = listItem->data(Qt::UserRole);

        if (itemData.isValid() && itemData.value<QGraphicsItem*>() == item)
        {
            delete ui->polygonList->takeItem(i);
            renumberPolygonList();
            break;
        }
    }
}

void MainWindow::on_actDelete_triggered()
{
    Document::Ptr doc = currentDocument();
    if (!doc)
        return;

    const QList<QListWidgetItem*> selectedItems = ui->polygonList->selectedItems();
    if (selectedItems.isEmpty())
        return;

    if (selectedItems.size() > 1)
    {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle(u8"Удаление фигур");
        msgBox.setText(
            QString(u8"Удалить выбранные фигуры?\nКоличество фигур: %1")
                .arg(selectedItems.size())
        );
        msgBox.setIcon(QMessageBox::Question);

        QPushButton* btnYes = msgBox.addButton(u8"Да", QMessageBox::AcceptRole);
        QPushButton* btnNo = msgBox.addButton(u8"Нет", QMessageBox::RejectRole);
        msgBox.setDefaultButton(btnNo);

        msgBox.exec();

        if (msgBox.clickedButton() != btnYes)
            return;
    }
    restoreTemporaryRaisedZValues();

    // Снимки для восстановления
    const QVector<ShapeBackup> backups = collectBackupsForItems(selectedItems);

    struct Payload
    {
        QVector<ShapeBackup>  backups;
        QVector<qulonglong>   uids;
    };

    std::shared_ptr<Payload> payload = std::make_shared<Payload>();
    payload->backups = backups;
    payload->uids.reserve(backups.size());

    for (const ShapeBackup& b : backups)
    {
        if (b.uid != 0)
            payload->uids.push_back(b.uid);
    }

    // redo: удалить текущие элементы по uid
    std::function<void()> redoFn = [this, payload]()
    {
        for (qulonglong uid : std::as_const(payload->uids))
        {
            if (!uid)
                continue;

            if (QGraphicsItem* gi = findItemByUid(uid))
            {
                _scene->removeItem(gi);
                removeListEntryBySceneItem(gi);
                clearLinePolylineStateForDeletedItem(gi);
                delete gi;
            }
        }

        if (Document::Ptr doc = currentDocument())
        {
            doc->isModified = true;
            updateFileListDisplay(doc->filePath);
        }
    };

    // undo: восстановить по снапшотам
    std::function<void()> undoFn = [this, payload]()
    {
        QVector<ShapeBackup> backups = payload->backups;

        std::sort(backups.begin(), backups.end(),
                  [](const ShapeBackup& a, const ShapeBackup& b)
        {
            return a.listRow < b.listRow;
        });

        for (const ShapeBackup& b : backups)
        {
            QGraphicsItem* created = recreateFromBackup(b);
            Q_UNUSED(created);
        }

        if (Document::Ptr doc = currentDocument())
        {
            doc->isModified = true;
            updateFileListDisplay(doc->filePath);
        }
    };

    if (doc->_undoStack)
        doc->_undoStack->push(new LambdaCommand(redoFn, undoFn, u8"Удаление фигур"));
}

void MainWindow::on_actSettingsApp_triggered()
{
    MainWindow::VisualStyle visualStyleBackup = _vstyle;
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
    init.labelFontPt   = _vstyle.labelFontPt;      // Размер и шрифт
    init.labelFont     = _vstyle.labelFont;

    init.handlePickRadius = _ghostPickRadius;
    init.edgePickRadius = _edgePickRadius;

    config::base().getValue("view.keep_image_scale_per_image", init.keepImageScale);
    config::base().getValue("ui.keep_menu_visibility", init.keepMenuBarVisibility);

    dlg.setValues(init);


    // connect(&dlg, &Settings::settingsApplied, this,
    //             [this](const Settings::Values& v)
    // {
    connect(&dlg, &Settings::settingsApplied, this,
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
        if (_scene)
            _scene->update();

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

                if (item == _videoRect
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

        if (_scene)
            onSceneSelectionChanged();

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
        visualStyleBackup = _vstyle;

        forEachScene([this](QGraphicsScene* sc)
        {
            if (!sc)
                return;

            sc->setProperty("fillShapeWhenSelected", _vstyle.fillShapeWhenSelected);
        });
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

            for (QGraphicsItem* item : sc->items())
            {
                if (!item)
                    continue;

                if (item == _videoRect
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

        if (_scene)
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
    QList<QGraphicsItem*> selectedItems = _scene->selectedItems();
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
    QList<QGraphicsItem*> selectedItems = _scene->selectedItems();
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
    QList<QGraphicsItem*> selectedItems = _scene->selectedItems();
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
    if (!_scene)
        return;

    QList<QGraphicsItem*> selectedItems = _scene->selectedItems();
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
    if (ui->polygonList->count() == 0)
        return;

    // Подтверждение
    QMessageBox msgBox(this);
    msgBox.setWindowTitle(u8"Сбросить разметку");
    msgBox.setText(u8"Удалить всю разметку на текущем снимке?");
    msgBox.setIcon(QMessageBox::Question);

    QPushButton* btnYes = msgBox.addButton(u8"Да",  QMessageBox::AcceptRole);
    QPushButton* btnNo  = msgBox.addButton(u8"Нет", QMessageBox::RejectRole);
    msgBox.setDefaultButton(btnNo);

    msgBox.exec();

    if (msgBox.clickedButton() != btnYes)
        return;

    // Собираем все элементы из списка
    QList<QListWidgetItem*> allItems;
    allItems.reserve(ui->polygonList->count());
    for (int i = 0; i < ui->polygonList->count(); ++i)
    {
        if (QListWidgetItem* it = ui->polygonList->item(i))
            allItems.push_back(it);
    }
    restoreTemporaryRaisedZValues();

    // Снимки для восстановления
    const QVector<ShapeBackup> backups = collectBackupsForItems(allItems);

    struct Payload
    {
        QVector<ShapeBackup>  backups;
        QVector<qulonglong>   uids;
    };

    std::shared_ptr<Payload> payload = std::make_shared<Payload>();
    payload->backups = backups;
    payload->uids.reserve(backups.size());

    for (const ShapeBackup& b : backups)
    {
        if (b.uid != 0)
            payload->uids.push_back(b.uid);
    }

    // redo: удалить все элементы по uid
    std::function<void()> redoFn = [this, payload]()
    {
        for (qulonglong uid : std::as_const(payload->uids))
        {
            if (!uid)
                continue;

            if (QGraphicsItem* gi = findItemByUid(uid))
            {
                _scene->removeItem(gi);
                removeListEntryBySceneItem(gi);
                clearLinePolylineStateForDeletedItem(gi);
                delete gi;
            }
        }

        if (Document::Ptr doc = currentDocument())
        {
            doc->isModified = true;
            updateFileListDisplay(doc->filePath);
        }
    };

    // undo: восстановить по снапшотам
    std::function<void()> undoFn = [this, payload]()
    {
        for (const ShapeBackup& b : std::as_const(payload->backups))
        {
            QGraphicsItem* created = recreateFromBackup(b);
            Q_UNUSED(created);
        }

        if (Document::Ptr doc = currentDocument())
        {
            doc->isModified = true;
            updateFileListDisplay(doc->filePath);
        }
    };

    if (doc->_undoStack)
        doc->_undoStack->push(new LambdaCommand(redoFn, undoFn, u8"Сброс разметки"));
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

    if (hasUnsaved || ui->polygonList->count() > 0)
    {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle(u8"Восстановить разметку");
        msgBox.setText(u8"Восстановить разметку из файла .yaml?\nВсе несохранённые изменения будут потеряны");
        msgBox.setIcon(QMessageBox::Question);

        QPushButton* btnYes = msgBox.addButton(u8"Да",  QMessageBox::AcceptRole);
        QPushButton* btnNo  = msgBox.addButton(u8"Нет", QMessageBox::RejectRole);
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
        QObject::connect(stack, &QUndoStack::indexChanged, this, [this, stack](int){
            if (_loadingNow) return;

            // реагируем только если этот стек активен у группы
            if (_undoGroup && (_undoGroup->activeStack() != stack)) return;

            if (Document::Ptr doc = currentDocument())
            {
                // Если стек в состоянии <пусто>, считаем файл неизмененным
                const bool clean = stack->isClean();
                doc->isModified = !clean;
                updateFileListDisplay(doc->filePath);
            }
        });

        // Убираем чекбоксы
        //item->setFlags(item->flags() & ~Qt::ItemIsUserCheckable);

        ui->fileList->addItem(item);
        if (doc->loadImage())
        {
            loadAnnotationFromFile(doc, false);
        }
    }
}

void MainWindow::updatePolygonListForCurrentScene()
{
    ui->polygonList->clear();

    if (ui->coordinateList)
        ui->coordinateList->clear();

    if (!_scene)
        return;

    QList<QGraphicsItem*> roots;
    for (QGraphicsItem* item : _scene->items())
    {
        if (!item)
            continue;

        if (item == _videoRect ||
            item == _tempRectItem ||
            item == _tempCircleItem ||
            item == _tempPolyline)
        {
            continue;
        }

        if (item->parentItem() != nullptr)
            continue;

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

    const QString oldClass = item->data(0).toString();

    if (_projectClasses.isEmpty())
        return;

    SelectClass dialog(_projectClasses, this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    const QString newClass = dialog.selectedClass();
    if (newClass.isEmpty() || newClass == oldClass)
        return;

    ShapeBackup before = makeBackupFromItem(item);
    const qulonglong uid = before.uid;

    item->setData(0, newClass);
    applyClassColorToItem(item, newClass);

    bool foundInList = false;
    for (int i = 0; i < ui->polygonList->count(); ++i)
    {
        QListWidgetItem* li = ui->polygonList->item(i);
        if (!li)
            continue;

        if (li->data(Qt::UserRole).value<QGraphicsItem*>() == item)
        {
            foundInList = true;
            break;
        }
    }
    if (!foundInList)
        linkSceneItemToList(item);
    else
        renumberPolygonList();

    ShapeBackup after = makeBackupFromItem(item);
    if (!sameGeometry(before, after))
        pushModifyShapeCommand(uid, before, after, u8"Смена класса");

    if (Document::Ptr doc = currentDocument(); doc && !doc->isModified)
    {
        doc->isModified = true;
        updateFileListDisplay(doc->filePath);
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

    // Пишем и JSON, и YAML в буфер обмена
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
            //rect->setRect(QRectF(x, y, width, height));
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
    if (!doc || !doc->scene)
        return;

    QJsonObject json = readShapesJsonFromClipboard();
    if (json.isEmpty())
        json = _shapesClipboard;

    if (json.isEmpty())
        return;

    QGraphicsScene* scene = doc->scene;

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

        if (!imageSize.isValid() && _videoRect)
            imageSize = _videoRect->pixmap().size();

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

        if (item == _videoRect ||
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

        if (item == _videoRect ||
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

    for (QGraphicsItem* item : newItems)
    {
        if (!item)
            continue;

        const QString className = item->data(0).toString();
        if (className.isEmpty())
            continue;

        QListWidgetItem* listItem = new QListWidgetItem;
        listItem->setData(Qt::UserRole, QVariant::fromValue(item));
        ui->polygonList->addItem(listItem);
        pastedListItems.append(listItem);
    }

    renumberPolygonListTextOnly();

    {
        QSignalBlocker blocker(ui->polygonList);
        ui->polygonList->clearSelection();

        for (QListWidgetItem* listItem : pastedListItems)
        {
            if (listItem)
                listItem->setSelected(true);
        }

        if (!pastedListItems.isEmpty() && pastedListItems.last())
            ui->polygonList->setCurrentItem(pastedListItems.last());
    }

    ui->polygonList->setFocus();

    QString descr = (newItems.count() == 1) ? u8"Вставка примитива"
                                            : u8"Вставка примитивов";

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

            // Важно: Line проверяем раньше Polyline.
            // Если Line наследуется от Polyline, иначе линия сохранится как polyline.
            if (qgraph::Line* line = dynamic_cast<qgraph::Line*>(item))
            {
                conf->setValue(yshape, "type", QString("line"));
                savePointsArray(conf, yshape, line->points());
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
    // Для внешних C/STD API на Windows используем нативную кодировку ОС
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
            messageBox(
                this,
                QMessageBox::Warning,
                u8"Ошибка при чтении базового файла с классами!"
            );
            //alog::stop();
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
            //alog::stop();
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

    // Сначала пытаемся прочитать новый формат:
    // classes:
    //   - name: ...
    //     color: ...
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

        for (QGraphicsItem* item : sc->items())
        {
            if (!item)
                continue;

            if (item == _videoRect
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

    if (_scene)
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
    if (!_scene || !ui || !ui->polygonList)
        return;

    if (_syncingSelection)
        return;

    _syncingSelection = true;

    // Обновляем цвета/заливку всех фигур с учетом текущего выделения
    for (QGraphicsItem* item : _scene->items())
    {
        if (!item)
            continue;

        if (item == _videoRect
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

    {
        QSignalBlocker blocker(ui->polygonList);
        ui->polygonList->clearSelection();

        const QList<QGraphicsItem*> selected = _scene->selectedItems();
        for (QGraphicsItem* sceneItem : selected)
        {
            if (!sceneItem)
                continue;

            if (qgraph::DragCircle* handle = dynamic_cast<qgraph::DragCircle*>(sceneItem))
            {
                if (handle->parentItem())
                    sceneItem = handle->parentItem();
            }

            for (int i = 0; i < ui->polygonList->count(); ++i)
            {
                QListWidgetItem* listItem = ui->polygonList->item(i);
                if (!listItem)
                    continue;

                QGraphicsItem* boundItem =
                    listItem->data(Qt::UserRole).value<QGraphicsItem*>();

                if (boundItem == sceneItem)
                {
                    listItem->setSelected(true);
                    break;
                }
            }
        }
    }

    updateCoordinateList();
    raiseSelectedShapesTemporarily();
    _scene->update();

    updateShapeListButtons();
    _syncingSelection = false;
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
    if (!item) return;

    // Удаляем из списка
    for (int i = 0; i < ui->polygonList->count(); ++i)
    {
        QListWidgetItem* listItem = ui->polygonList->item(i);
        if (listItem && listItem->data(Qt::UserRole).value<QGraphicsItem*>() == item)
        {
            delete ui->polygonList->takeItem(i);
            renumberPolygonList();
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
    if (Document::Ptr doc = currentDocument())
    {
        doc->isModified = true;
        updateFileListDisplay(doc->filePath);
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
        clearLinePolylineStateForDeletedItem(sceneItem);
        delete sceneItem;
    }
    // Удаляем из списка
    delete ui->polygonList->takeItem(ui->polygonList->row(item));
    renumberPolygonList();
    // Помечаем документ как измененный
    if (Document::Ptr doc = currentDocument())
    {
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

    QString className = sceneItem->data(0).toString();
    if (className.isEmpty())
        return;

    ensureShapeNumber(sceneItem);

    QListWidgetItem* listItem = new QListWidgetItem;
    listItem->setData(Qt::UserRole, QVariant::fromValue(sceneItem));

    if (row < 0 || row > ui->polygonList->count())
        ui->polygonList->addItem(listItem);
    else
        ui->polygonList->insertItem(row, listItem);

    if (needRenumber)
        renumberPolygonList();
    else
        updatePolygonListItemText(listItem);
}

void MainWindow::renumberPolygonList()
{
    if (!ui || !ui->polygonList)
        return;

    refreshShapeListOrderRole();

    for (int i = 0; i < ui->polygonList->count(); ++i)
        updatePolygonListItemText(ui->polygonList->item(i));

    updateShapeListButtons();

    if (_scene)
        _scene->update();
}

void MainWindow::renumberPolygonListTextOnly()
{
    if (!ui || !ui->polygonList)
        return;

    refreshShapeListOrderRole();

    for (int i = 0; i < ui->polygonList->count(); ++i)
        updatePolygonListItemText(ui->polygonList->item(i));

    updateShapeListButtons();
}

void MainWindow::showPolygonListContextMenu(const QPoint &pos)
{
    QListWidgetItem* item = ui->polygonList->itemAt(pos);
    if (!item) return;

    QGraphicsItem* sceneItem = item->data(Qt::UserRole).value<QGraphicsItem*>();
    if (!sceneItem) return;

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
        if (!item->isSelected())
        {
            ui->polygonList->clearSelection();
            item->setSelected(true);
        }

        ui->polygonList->setCurrentItem(item);
        on_actDelete_triggered();
    }
}

void MainWindow::updatePolygonListItemText(QListWidgetItem* item)
{
    if (!item)
        return;

    QGraphicsItem* sceneItem = item->data(Qt::UserRole).value<QGraphicsItem*>();
    if (!sceneItem)
        return;

    const QString className = sceneItem->data(0).toString();

    const int number = ensureShapeNumber(sceneItem);
    QString text = QString("%1: %2").arg(number).arg(className);
    if (!sceneItem->isVisible())
        text += " [скрыто]";

    item->setText(text);
}

void MainWindow::updatePolygonListTexts()
{
    if (!ui || !ui->polygonList)
        return;

    for (int i = 0; i < ui->polygonList->count(); ++i)
        updatePolygonListItemText(ui->polygonList->item(i));
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
    if (!_scene) return;

    // Проходим по всем элементам и поднимаем их ручки
    for (QGraphicsItem* item : _scene->items())
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
            point->raiseHandlesToTop();
    }
}

bool MainWindow::isRootShapeItem(QGraphicsItem* item) const
{
    if (!item)
        return false;

    if (item == _videoRect ||
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

    if (_scene)
        _scene->update();
}

void MainWindow::raiseSelectedShapesTemporarily()
{
    if (!_scene)
        return;

    restoreTemporaryRaisedZValues();

    QList<QGraphicsItem*> selectedRoots;

    qreal maxZ = _videoRect ? _videoRect->zValue() : 0.0;

    for (QGraphicsItem* item : _scene->items())
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

    if (_scene)
        _scene->update();
}

void MainWindow::moveSelectedItemsToBack()
{
    if (!_scene)
        return;

    restoreTemporaryRaisedZValues();

    QList<QGraphicsItem*> selectedRoots;
    QList<QGraphicsItem*> nonSelectedRoots;

    std::function<bool(QGraphicsItem*)> isRootShape = [this](QGraphicsItem* item) -> bool
    {
        if (!item)
            return false;

        if (item == _videoRect ||
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

    for (QGraphicsItem* item : _scene->items())
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

    // Выбранные фигуры уходят в самый низ среди фигур.
    // Их внутренний относительный порядок сохраняется.
    orderedRoots.append(selectedRoots);
    orderedRoots.append(nonSelectedRoots);

    const qreal imageZ = (_videoRect ? _videoRect->zValue() : 0.0);
    qreal z = imageZ + 1.0;

    for (QGraphicsItem* item : orderedRoots)
    {
        if (item)
            item->setZValue(z++);
    }

    {
        QSignalBlocker blockScene(_scene);
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

    if (_scene)
        _scene->update();
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
    //QDialog dialog(this);
    QDialog dialog(
        this,
        Qt::Dialog
        |Qt::CustomizeWindowHint
        |Qt::WindowTitleHint
        |Qt::WindowCloseButtonHint);

    dialog.setWindowTitle(u8"Несохраненные изменения");
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

    QVBoxLayout* mainLayout = new QVBoxLayout(&dialog);

    QLabel* messageLabel = new QLabel(u8"Следующие файлы имеют несохраненные изменения:\nХотите сохранить изменения?");
    mainLayout->addWidget(messageLabel);

    // Список файлов
    QListWidget* fileList = new QListWidget();
    for (const Document::Ptr& doc : unsavedDocs)
    {
        QFileInfo fileInfo(doc->filePath);
        QListWidgetItem* item = new QListWidgetItem(fileInfo.fileName());
        //item->setData(Qt::UserRole, QVariant::fromValue(doc));
        item->setData(Qt::UserRole, doc->filePath);
        fileList->addItem(item);
        item->setSelected(true);
    }
    mainLayout->addWidget(fileList);

    // Кнопки
    QDialogButtonBox* buttonBox = new QDialogButtonBox();

    QPushButton* saveAllButton = new QPushButton(u8"Сохранить все");
    QPushButton* discardAllButton = new QPushButton(u8"Не сохранять");
    QPushButton* cancelButton = new QPushButton(u8"Отмена");

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
    topIsHandle = false;
    if (!_scene) return nullptr;

    GraphicsView* view = ui ? ui->graphView : nullptr;
    const qreal radiusScene = viewPixelsToSceneRadius(view, effectiveViewPickPx(_ghostPickRadius));
    if (radiusScene <= 0.0)
      return nullptr;

    const QRectF pickRect(scenePos - QPointF(radiusScene, radiusScene),
                        QSizeF(2 * radiusScene, 2 * radiusScene));

    const QList<QGraphicsItem*> items = _scene->items(pickRect, Qt::IntersectsItemShape, Qt::DescendingOrder);

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
        if (qgraph::Shape* shape = dynamic_cast<qgraph::Shape*>(_ghostTarget->parentItem()))
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
    //const qreal scale = 1;
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
    _ghostActive = true;
    _ghostHover  = false;

    _ghostGrabOffset = scenePos - _ghostHandle->pos();
    _ghostTarget->setGhostDriven(true);
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
    if (!_scene)
        return nullptr;

    GraphicsView* view = ui ? ui->graphView : nullptr;
    const qreal pickRadius = viewPixelsToSceneRadius(view, effectiveViewPickPx(customRadius));
    if (pickRadius <= 0.0)
        return nullptr;

    const QRectF probe(scenePos - QPointF(pickRadius, pickRadius),
                       QSizeF(pickRadius * 2, pickRadius * 2));

    qgraph::DragCircle* best = nullptr;
    qreal bestD2 = std::numeric_limits<qreal>::max();

    for (QGraphicsItem* item : _scene->items(probe, Qt::IntersectsItemShape))
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

    const QList<QGraphicsItem*> items =
        _scene->items(probe, Qt::IntersectsItemBoundingRect, Qt::DescendingOrder);

    for (QGraphicsItem* item : items)
    {
        if (!item || !item->isVisible() || !item->scene())
            continue;

        if (item == _videoRect)
            continue;

        if (dynamic_cast<qgraph::DragCircle*>(item))
            continue;

        // Берем только фигуры
        const bool isShape =
                (dynamic_cast<qgraph::Polyline*>(item)  != nullptr)
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

void MainWindow::startHandleDrag(qgraph::DragCircle* handle, const QPointF& scenePos)
{    
    _m_isDraggingHandle = true;
    _m_dragHandle = handle;
    _handleDragging = true; // Начало перетаскивания ручки
    updateModeLabel();

    // UNDO: запомним владельца и снимок "до"
    _handleEditedItem = handle ? handle->parentItem() : nullptr;
    _handleDragHadChanges = false;
    if (_handleEditedItem)
    {
        _handleBeforeSnap = makeBackupFromItem(_handleEditedItem);
    }

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
        Document::Ptr doc = currentDocument();
        if (doc && !doc->isModified)
        {
            doc->isModified = true;
            updateFileListDisplay(doc->filePath);
        }
        _m_dragHandle->restoreBaseStyle();
    }
    // UNDO: если что-то реально поменяли - пушим команду
    if (_handleEditedItem && _handleDragHadChanges)
    {
        ShapeBackup after = makeBackupFromItem(_handleEditedItem);
        if (!sameGeometry(_handleBeforeSnap, after))
        {
            // красивое имя действия
            QString what = u8"Перемещение узла";
            pushHandleEditCommand(_handleEditedItem, _handleBeforeSnap, after, what);
        }
    }
    _handleEditedItem = nullptr;
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
    //const qreal scale = 1;
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
    if (!_scene) return;

    for (QGraphicsItem* item : _scene->items())
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
    if (_scene)
    {
        _scene->setProperty("fillShapeWhenSelected", _vstyle.fillShapeWhenSelected);

        apply_LineWidth_ToScene(_scene);
        apply_PointSize_ToScene(_scene);
        apply_NumberSize_ToScene(_scene);
        updateLineColorsForScene(_scene);

        for (QGraphicsItem* item : _scene->items())
        {
            apply_PointStyle_ToItem(item);

            const QString cls = item ? item->data(0).toString() : QString();
            if (!cls.isEmpty())
                applyClassColorToItem(item, cls);
        }
        _scene->update();
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
    //using SDV = Settings::Values;
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
    //using SDV = Settings::Values;
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

ShapeBackup MainWindow::makeBackupFromItem(QGraphicsItem* graphicsItem) const
{
    ShapeBackup backup{};

    if (!graphicsItem)
        return backup;

    backup.uid = graphicsItem->data(_roleUid).toULongLong();
    if (backup.uid == 0)
        backup.uid = ensureUid(graphicsItem);

    backup.shapeNumber = ensureShapeNumber(graphicsItem);
    backup.className = graphicsItem->data(0).toString();
    backup.zValue = originalZValueForItem(graphicsItem);
    backup.visible = graphicsItem->isVisible();

    if (qgraph::Rectangle* rect = dynamic_cast<qgraph::Rectangle*>(graphicsItem))
    {
        backup.kind = ShapeKind::Rectangle;
        backup.rect = rect->mapRectToScene(rect->rect());
    }
    else if (qgraph::Circle* circle = dynamic_cast<qgraph::Circle*>(graphicsItem))
    {
        backup.kind = ShapeKind::Circle;
        backup.circleCenter = circle->center();
        backup.circleRadius = circle->realRadius();
    }
    else if (qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(graphicsItem))
    {
        backup.kind  = ShapeKind::Polyline;
        backup.points = polyline->points();
        backup.closed = polyline->isClosed();
    }
    else if (qgraph::Line* line = dynamic_cast<qgraph::Line*>(graphicsItem))
    {
        backup.kind  = ShapeKind::Line;
        backup.points = line->points();
        backup.closed = line->isClosed();
        backup.numberingFromLast = line->isNumberingFromLast();
    }
    else if (qgraph::Point* point = dynamic_cast<qgraph::Point*>(graphicsItem))
    {
        backup.kind = ShapeKind::Point;
        backup.pointCenter = point->center();
    }

    backup.listRow = -1;
    backup.sceneRow = -1;
    if (ui && ui->polygonList && graphicsItem)
    {
        for (int i = 0; i < ui->polygonList->count(); ++i)
        {
            QListWidgetItem* listWidget = ui->polygonList->item(i);
            if (!listWidget)
                continue;

            if (listWidget->data(Qt::UserRole).value<QGraphicsItem*>() == graphicsItem)
            {
                backup.listRow = i;
                backup.sceneRow = i;
                break;
            }
        }
    }
    return backup;
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

        out.append(makeBackupFromItem(gi));
    }
    return out;
}

QGraphicsItem* MainWindow::recreateFromBackup(const ShapeBackup& backup)
{
    QGraphicsItem* created = nullptr;

    std::function<void(QGraphicsItem*, qulonglong)> applyBackupUid =
        [this](QGraphicsItem* item, qulonglong uid)
    {
        if (!item)
            return;

        if (uid != 0)
        {
            // Восстанавливаем сохраненный uid.
            item->setData(_roleUid, QVariant::fromValue<qulonglong>(uid));
            if (uid >= _uidCounter) // Чтобы новые фигуры не получили такой же
                _uidCounter = uid + 1;
        }
        else
            ensureUid(item);
    };
    // Восстанавливаем номер фигуры, который отображается в списке
    std::function<void(QGraphicsItem*, int)> applyBackupNumber =
        [this](QGraphicsItem* item, int number)
    {
        if (!item)
            return;

        if (number >= 0)
            item->setData(_roleShapeNumber, number);
        else
            ensureShapeNumber(item);
    };

    switch (backup.kind)
    {
        case ShapeKind::Rectangle:
        {
            qgraph::Rectangle* rect = new qgraph::Rectangle(_scene);

            applyBackupUid(rect, backup.uid);
            rect->setRealSceneRect(backup.rect);
            rect->updatePointNumbers();
            apply_LineWidth_ToItem(rect);
            apply_PointSize_ToItem(rect);
            apply_NumberSize_ToItem(rect);
            rect->setData(0, backup.className);
            applyClassColorToItem(rect, backup.className);
            applyBackupNumber(rect, backup.shapeNumber);
            rect->setZValue(backup.zValue);
            rect->setVisible(backup.visible);
            linkSceneItemToList(rect, backup.listRow);
            created = rect;
            break;
        }
        case ShapeKind::Circle:
        {
            qgraph::Circle* circle = new qgraph::Circle(_scene, backup.circleCenter);

            applyBackupUid(circle, backup.uid);
            circle->setRealRadius(backup.circleRadius);
            apply_LineWidth_ToItem(circle);
            apply_PointSize_ToItem(circle);
            circle->setData(0, backup.className);
            applyClassColorToItem(circle, backup.className);
            applyBackupNumber(circle, backup.shapeNumber);
            circle->setZValue(backup.zValue);
            circle->setVisible(backup.visible);
            circle->updateHandlePosition();
            linkSceneItemToList(circle, backup.listRow);
            created = circle;
            break;
        }
        case ShapeKind::Polyline:
        {
            if (backup.points.isEmpty())
                break;

            qgraph::Polyline* polyline = new qgraph::Polyline(_scene, backup.points.front());

            applyBackupUid(polyline, backup.uid);
            polyline->beginBulkLoad();
            for (int i = 1; i < backup.points.size(); ++i)
                polyline->addPoint(backup.points[i], _scene);

            polyline->endBulkLoad();

            if (backup.closed)
                polyline->closePolyline();

            apply_LineWidth_ToItem(polyline);
            apply_PointSize_ToItem(polyline);
            apply_NumberSize_ToItem(polyline);

            polyline->setData(0, backup.className);
            applyClassColorToItem(polyline, backup.className);
            applyBackupNumber(polyline, backup.shapeNumber);
            polyline->setZValue(backup.zValue);
            polyline->setVisible(backup.visible);
            polyline->updateHandlePosition();
            linkSceneItemToList(polyline, backup.listRow);
            created = polyline;
            break;
        }
        case ShapeKind::Line:
        {
            if (backup.points.isEmpty())
                break;
            qgraph::Line* line = new qgraph::Line(_scene, backup.points.front());
            applyBackupUid(line, backup.uid);

            line->beginBulkLoad();
            for (int i = 1; i < backup.points.size(); ++i)
                line->addPoint(backup.points[i], _scene);

            line->endBulkLoad();

            if (backup.closed)
                line->closeLine();

            apply_LineWidth_ToItem(line);
            apply_PointSize_ToItem(line);
            apply_NumberSize_ToItem(line);

            line->setData(0, backup.className);
            applyClassColorToItem(line, backup.className);
            applyBackupNumber(line, backup.shapeNumber);
            line->setZValue(backup.zValue);
            line->setVisible(backup.visible);
            line->updateHandlePosition();
            linkSceneItemToList(line, backup.listRow);
            created = line;
            break;
        }
        case ShapeKind::Point:
        {
            qgraph::Point* point = new qgraph::Point(_scene);

            applyBackupUid(point, backup.uid);
            apply_PointSize_ToItem(point);
            apply_NumberSize_ToItem(point);
            apply_PointStyle_ToItem(point);
            point->setCenter(backup.pointCenter);
            point->setData(0, backup.className);
            applyClassColorToItem(point, backup.className);
            applyBackupNumber(point, backup.shapeNumber);
            point->setZValue(backup.zValue);
            point->setVisible(backup.visible);
            linkSceneItemToList(point, backup.listRow);
            created = point;
            break;
        }
    }

    if (created)
        created->setFocus();

    if (created)
        created->setData(_roleUid, QVariant::fromValue<qulonglong>(backup.uid));

    return created;
}

void MainWindow::applyBackupToExisting(QGraphicsItem* item, const ShapeBackup& backup)
{
    if (!item)
        return;

    if (backup.shapeNumber >= 0)
        item->setData(_roleShapeNumber, backup.shapeNumber);

    switch (backup.kind)
    {
        case ShapeKind::Rectangle:
            if (qgraph::Rectangle* rect = dynamic_cast<qgraph::Rectangle*>(item))
            {
                rect->setRealSceneRect(backup.rect);
                rect->updatePointNumbers();
                apply_LineWidth_ToItem(rect);
                apply_PointSize_ToItem(rect);
                apply_NumberSize_ToItem(rect);
                rect->setData(0, backup.className);
                applyClassColorToItem(rect, backup.className);
                rect->setZValue(backup.zValue);
                rect->setVisible(backup.visible);
                rect->updateHandlePosition();
            }
            break;

        case ShapeKind::Circle:
            if (qgraph::Circle* circle = dynamic_cast<qgraph::Circle*>(item))
            {
                circle->setRealRadius(backup.circleRadius);
                apply_LineWidth_ToItem(circle);
                apply_PointSize_ToItem(circle);
                circle->setData(0, backup.className);
                applyClassColorToItem(circle, backup.className);
                circle->setZValue(backup.zValue);
                circle->setVisible(backup.visible);

                const QPointF cur = circle->sceneBoundingRect().center();
                const QPointF dst = QPointF(backup.circleCenter);
                const QPointF d = dst - cur;
                if (!qFuzzyIsNull(d.x()) || !qFuzzyIsNull(d.y()))
                    circle->moveBy(d.x(), d.y());

                circle->updateHandlePosition();
            }
            break;

        case ShapeKind::Polyline:
        {
            if (qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(item))
            {
                // Полная замена узлов из снапшота
                polyline->replaceScenePoints(backup.points, backup.closed);
                polyline->setData(0, backup.className);
                polyline->setVisible(backup.visible);
                polyline->setZValue(backup.zValue);
                apply_LineWidth_ToItem(polyline);
                apply_PointSize_ToItem(polyline);
                apply_NumberSize_ToItem(polyline);
                applyClassColorToItem(polyline, backup.className);
            }
            break;
        }

        case ShapeKind::Line:
        {
            if (qgraph::Line* line = dynamic_cast<qgraph::Line*>(item))
            {
                line->replaceScenePoints(backup.points, backup.closed);
                line->setNumberingFromLast(backup.numberingFromLast);
                line->setData(0, backup.className);
                line->setVisible(backup.visible);
                line->setZValue(backup.zValue);
                apply_LineWidth_ToItem(line);
                apply_PointSize_ToItem(line);
                apply_NumberSize_ToItem(line);
                applyClassColorToItem(line, backup.className);
            }
            break;
        }
        case ShapeKind::Point:
            if (qgraph::Point* point = dynamic_cast<qgraph::Point*>(item))
            {
                apply_PointStyle_ToItem(point);
                apply_PointSize_ToItem(point);
                apply_NumberSize_ToItem(point);
                point->setCenter(backup.pointCenter);
                point->setData(0, backup.className);
                applyClassColorToItem(point, backup.className);
                point->setZValue(backup.zValue);
                point->setVisible(backup.visible);
                point->updateHandlePosition();
            }
            break;
    }
    if (ui && ui->polygonList)
    {
        int currentRow = -1;
        QListWidgetItem* currentItem = nullptr;

        for (int i = 0; i < ui->polygonList->count(); ++i)
        {
            QListWidgetItem* li = ui->polygonList->item(i);
            if (!li)
                continue;

            if (li->data(Qt::UserRole).value<QGraphicsItem*>() == item)
            {
                currentRow = i;
                currentItem = li;
                break;
            }
        }

        if (!currentItem && !backup.className.isEmpty())
        {
            linkSceneItemToList(item, backup.listRow);
        }
        else if (currentItem)
        {
            const int targetRow = (backup.listRow < 0) ? currentRow : backup.listRow;
            if (targetRow != currentRow &&
                targetRow >= 0 &&
                targetRow < ui->polygonList->count())
            {
                QListWidgetItem* moved = ui->polygonList->takeItem(currentRow);
                ui->polygonList->insertItem(targetRow, moved);
            }
            renumberPolygonList();
        }
    }
}

void MainWindow::pushAdoptExistingShapeCommand(QGraphicsItem* createdNow,
                                               const ShapeBackup& backup,
                                               const QString& description)
{
    struct Payload
    {
        ShapeBackup backup;
        qulonglong  uid = 0;
        bool skipFirstRedo = false;
    };

    std::shared_ptr<Payload> payload = std::make_shared<Payload>();
    payload->backup = backup;

    // Берем uid из снапшота или с самого объекта
    qulonglong uid = backup.uid;
    if (uid == 0 && createdNow)
        uid = createdNow->data(_roleUid).toULongLong();
    if (uid == 0 && createdNow)
        uid = ensureUid(createdNow);
    payload->uid = uid;
    payload->skipFirstRedo = (createdNow != nullptr);

    std::function<void()> redoFn = [this, payload]()
    {
        if (payload->uid == 0)
            return;

        // После QUndoStack::push() redo вызывается сразу.
        // Фигура уже существует и уже в нужном состоянии,
        // поэтому первый redo пропускаем.
        if (payload->skipFirstRedo)
        {
            payload->skipFirstRedo = false;

            if (Document::Ptr doc = currentDocument())
            {
                doc->isModified = true;
                updateFileListDisplay(doc->filePath);
            }
            return;
        }

        // Пытаемся найти уже существующий объект по uid
        QGraphicsItem* item = findItemByUid(payload->uid);
        if (item)
        {
            // Фигура есть - просто приводим ее к состоянию из снапшота
            applyBackupToExisting(item, payload->backup);
        }
        else
        {
            // Фигуры нет - создаем по снапшоту
            QGraphicsItem* created = recreateFromBackup(payload->backup);
            if (created && payload->uid != 0)
            {
                created->setData(_roleUid,
                                 QVariant::fromValue<qulonglong>(payload->uid));
            }
        }

        if (Document::Ptr doc = currentDocument())
        {
            doc->isModified = true;
            updateFileListDisplay(doc->filePath);
        }
    };

    std::function<void()> undoFn = [this, payload]()
    {
        if (payload->uid == 0)
            return;

        if (QGraphicsItem* item = findItemByUid(payload->uid))
        {
            if (QGraphicsScene* scene = item->scene())
                scene->removeItem(item);
            removeListEntryBySceneItem(item);
            // Нужно сбросить временные указатели и флаги,
            // без этого после undo возможны обращения к удаленному объекту
            clearLinePolylineStateForDeletedItem(item);
            delete item;
        }

        if (Document::Ptr doc = currentDocument())
        {
            doc->isModified = true;
            updateFileListDisplay(doc->filePath);
        }
    };

    if (QUndoStack* stack = activeUndoStack())
        stack->push(new LambdaCommand(redoFn, undoFn, description));
}

void MainWindow::pushCreateShapeCommand(const ShapeBackup& backup, const QString& description)
{
    struct Payload
    {
        ShapeBackup backup;
        qulonglong  uid = 0;
    };

    std::shared_ptr<Payload> payload = std::make_shared<Payload>();
    payload->backup = backup;
    payload->uid = backup.uid;

    std::function<void()> redoFn = [this, payload]()
    {
        // Ищем существующий объект по uid
        QGraphicsItem* it = nullptr;
        if (payload->uid != 0)
            it = findItemByUid(payload->uid);

        if (it)
        {
            // Фигура уже есть
            applyBackupToExisting(it, payload->backup);
        }
        else
        {
            // Фигуры нет, создаем по снапшоту
            it = recreateFromBackup(payload->backup);
            if (it && payload->uid != 0)
            {
                it->setData(_roleUid, QVariant::fromValue<qulonglong>(payload->uid));
            }
        }

        if (Document::Ptr doc = currentDocument())
        {
            doc->isModified = true;
            updateFileListDisplay(doc->filePath);
        }
    };

    std::function<void()> undoFn = [this, payload]()
    {
        if (payload->uid == 0)
            return;

        if (QGraphicsItem* item = findItemByUid(payload->uid))
        {
            _scene->removeItem(item);
            removeListEntryBySceneItem(item);
            clearLinePolylineStateForDeletedItem(item);
            delete item;
        }

        if (Document::Ptr doc = currentDocument())
        {
            doc->isModified = true;
            updateFileListDisplay(doc->filePath);
        }
    };

    if (QUndoStack* stack = activeUndoStack())
        stack->push(new LambdaCommand(redoFn, undoFn, description));
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
        bool skipFirstRedo = false;
    };

    std::shared_ptr<Payload> payload = std::make_shared<Payload>();
    payload->before = before;
    payload->after  = after;

    qulonglong uid = 0;
    if (item)
        uid = item->data(_roleUid).toULongLong();
    if (uid == 0 && item)
        uid = ensureUid(item);
    payload->uid = uid;

    std::function<bool(qulonglong, const ShapeBackup&)> apply =
        [this](qulonglong uid, const ShapeBackup& snap)
    {
        QGraphicsItem* item = findItemByUid(uid);
        if (item)
        {
            applyBackupToExisting(item, snap);
            return true;
        }
        QGraphicsItem* created = recreateFromBackup(snap);
        if (created)
        {
            created->setData(_roleUid, QVariant::fromValue<qulonglong>(snap.uid ? snap.uid : uid));
            return true;
        }
        return false;
    };

    std::function<void()> redoFn = [this, payload, apply]()
    {
        apply(payload->uid, payload->after);
        if (Document::Ptr doc = currentDocument())
        {
            doc->isModified = true;
            updateFileListDisplay(doc->filePath);
        }
    };

    std::function<void()> undoFn = [this, payload, apply]()
    {
        apply(payload->uid, payload->before);
        if (Document::Ptr doc = currentDocument())
        {
            doc->isModified = true;
            updateFileListDisplay(doc->filePath);
        }
    };

    if (QUndoStack* stack = activeUndoStack())
        stack->push(new LambdaCommand(redoFn, undoFn, description));
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

    std::shared_ptr<Payload> payload = std::make_shared<Payload>();
    payload->before = before;
    payload->after = after;

    // Сохраняем uid
    qulonglong uid = 0;
    if (item)
        uid = item->data(_roleUid).toULongLong();
    if (uid == 0 && item)
        uid = ensureUid(item);
    payload->uid = uid;

    // Найти актуальный item по uid; если нет - пересоздать из снапшота
    std::function<bool(qulonglong, const ShapeBackup&)> apply =
        [this](qulonglong uid, const ShapeBackup& snap)
    {
        QGraphicsItem* item = findItemByUid(uid);
        if (item)
        {
            applyBackupToExisting(item, snap);
            return true;
        }
        // Не нашли - пересоздаем (и привязываем тот же uid)
        QGraphicsItem* created = recreateFromBackup(snap);
        if (created)
        {
            created->setData(_roleUid, QVariant::fromValue<qulonglong>(snap.uid ? snap.uid : uid));
            return true;
        }
        return false;
    };

    std::function<void()> redoFn = [this, payload, apply]()
    {
        apply(payload->uid, payload->after);
        if (Document::Ptr doc = currentDocument())
        {
            doc->isModified = true;
            updateFileListDisplay(doc->filePath);
        }
    };

    std::function<void()> undoFn = [this, payload, apply]()
    {
        apply(payload->uid, payload->before);
        if (Document::Ptr doc = currentDocument())
        {
            doc->isModified = true;
            updateFileListDisplay(doc->filePath);
        }
    };

    if (QUndoStack* stack = activeUndoStack())
        stack->push(new LambdaCommand(redoFn, undoFn, description));
}

void MainWindow::pushModifyShapeCommand(qulonglong uid,
                                        const ShapeBackup& before,
                                        const ShapeBackup& after,
                                        const QString& description)
{
    Document::Ptr doc = currentDocument();
    if (!doc || !doc->_undoStack)
        return;

    struct Payload
    {
        qulonglong uid;
        ShapeBackup before;
        ShapeBackup after;
    };

    std::shared_ptr<Payload> payload = std::make_shared<Payload>();
    payload->uid    = uid;
    payload->before = before;
    payload->after  = after;

    // Применяем снапшот к существующей фигуре по uid.
    // Если фигуру не нашли - просто no-op, без пересоздания.
    std::function<void(qulonglong, const ShapeBackup&)> applySnap =
        [this](qulonglong uid, const ShapeBackup& snap)
    {
        if (QGraphicsItem* item = findItemByUid(uid))
        {
            applyBackupToExisting(item, snap);
        }
    };

    std::function<void()> redoFn = [this, payload, applySnap]()
    {
        applySnap(payload->uid, payload->after);
        if (Document::Ptr doc = currentDocument())
        {
            doc->isModified = true;
            updateFileListDisplay(doc->filePath);
        }
    };

    std::function<void()> undoFn = [this, payload, applySnap]()
    {
        applySnap(payload->uid, payload->before);
        if (Document::Ptr doc = currentDocument())
        {
            doc->isModified = true;
            updateFileListDisplay(doc->filePath);
        }
    };

    doc->_undoStack->push(new LambdaCommand(redoFn, undoFn, description));
}

void MainWindow::pushMoveImageCommand(const QPointF& before,
                                      const QPointF& after,
                                      const QString& description)
{
    Document::Ptr doc = currentDocument();
    if (!doc || !doc->_undoStack || !_videoRect)
        return;

    if (before == after)
        return;

    struct Payload
    {
        QPointF before;
        QPointF after;
    };

    std::shared_ptr<Payload> payload = std::make_shared<Payload>();
    payload->before = before;
    payload->after = after;

    std::function<void(const QPointF&)> applyPos =
        [this](const QPointF& pos)
    {
        if (_videoRect)
            _videoRect->setPos(pos);
    };

    std::function<void()> redoFn = [this, payload, applyPos]()
    {
        applyPos(payload->after);
        if (Document::Ptr doc = currentDocument())
        {
            doc->isModified = true;
            updateFileListDisplay(doc->filePath);
        }
    };

    std::function<void()> undoFn = [this, payload, applyPos]()
    {
        applyPos(payload->before);
        if (Document::Ptr doc = currentDocument())
        {
            doc->isModified = true;
            updateFileListDisplay(doc->filePath);
        }
    };

    doc->_undoStack->push(new LambdaCommand(redoFn, undoFn, description));
}

void MainWindow::pushReplaceShapeCommand(qulonglong uid,
                                        const ShapeBackup& before,
                                        const ShapeBackup& after,
                                        const QString& description)
{
    Document::Ptr doc = currentDocument();
    if (!doc || !doc->_undoStack)
        return;

    struct Payload
    {
        qulonglong uid = 0;
        ShapeBackup before;
        ShapeBackup after;
    };

    std::shared_ptr<Payload> payload = std::make_shared<Payload>();
    payload->uid = uid;
    payload->before = before;
    payload->after  = after;

    payload->before.uid = uid;
    payload->after.uid  = uid;

    std::function<void(qulonglong, const ShapeBackup&)> replaceBySnap =
        [this](qulonglong uid, const ShapeBackup& snap)
    {
        if (!uid)
            return;

        if (QGraphicsItem* item = findItemByUid(uid))
        {
            if (QGraphicsScene* sc = item->scene())
                sc->removeItem(item);

            removeListEntryBySceneItem(item);
            clearLinePolylineStateForDeletedItem(item);
            delete item;
        }

        QGraphicsItem* created = recreateFromBackup(snap);
        if (created)
            created->setData(_roleUid, QVariant::fromValue<qulonglong>(uid));
    };

    std::function<void()> redoFn = [this, payload, replaceBySnap]()
    {
        replaceBySnap(payload->uid, payload->after);
        if (Document::Ptr doc = currentDocument())
        {
            doc->isModified = true;
            updateFileListDisplay(doc->filePath);
        }
    };

    std::function<void()> undoFn = [this, payload, replaceBySnap]()
    {
        replaceBySnap(payload->uid, payload->before);
        if (Document::Ptr doc = currentDocument())
        {
            doc->isModified = true;
            updateFileListDisplay(doc->filePath);
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
            clearLinePolylineStateForDeletedItem(gi);
            delete gi;
        }
        // Удаляем сам элемент списка
        delete ui->polygonList->takeItem(ui->polygonList->row(li));
    }
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
            renumberPolygonList();
            return;
        }
    }
}

QGraphicsItem* MainWindow::sceneItemFromListItem(const QListWidgetItem* listItem)
{
    if (!listItem)
        return nullptr;
    return listItem->data(Qt::UserRole).value<QGraphicsItem*>();
}

QGraphicsItem* MainWindow::findItemByUid(qulonglong uid) const
{
    if (!_scene || uid == 0)
        return nullptr;

    const QList<QGraphicsItem*> itemsList = _scene->items();
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

    updateModeLabel();
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

bool MainWindow::performMergeLines(qgraph::Line* lineA, int indexA, qgraph::Line* lineB, int indexB)
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

    struct Payload
    {
        typedef shared_ptr<Payload> Ptr;
        //typedef container_ptr<Payload> Ptr;

        ShapeBackup aBackup;
        ShapeBackup bBackup;
        QVector<QPointF> mergedPts;
        QString label;
        qulonglong mergedUid = 0;
        qreal z = 0.0;
        bool numberingFromLast = false;
    };

    Payload::Ptr payload = make_shared<Payload>();
    //Payload::Ptr payload = Payload::Ptr::create();

    std::function<ShapeBackup(QGraphicsItem*)> backupForSceneItem =
        [this](QGraphicsItem* gi) -> ShapeBackup
    {
        ShapeBackup backup{};
        backup.uid = gi->data(_roleUid).toULongLong();
        if (backup.uid == 0)
            backup.uid = ensureUid(gi);
        backup.className = gi->data(0).toString();
        backup.zValue = gi->zValue();
        backup.visible = gi->isVisible();

        if (qgraph::Line* line = dynamic_cast<qgraph::Line*>(gi))
        {
            backup.kind = ShapeKind::Line;
            backup.points = line->points();
            backup.closed = false;
        }
        return backup;
    };

    payload->numberingFromLast = numberingFromLast;
    payload->aBackup = backupForSceneItem(lineA);
    payload->bBackup = backupForSceneItem(lineB);
    payload->mergedPts = merged;
    payload->label = outLabel;
    payload->z = qMax(lineA->zValue(), lineB->zValue());

    payload->mergedUid = ++_uidCounter; // Резервируем uid

    std::function<void()> redoFn = [this, payload]()
    {
        // Удаляем А и В по uid
        for (qulonglong uid : {payload->aBackup.uid, payload->bBackup.uid})
        {
            if (!uid) continue;
            if (QGraphicsItem* gi = findItemByUid(uid))
            {
                _scene->removeItem(gi);
                removeListEntryBySceneItem(gi);
                clearLinePolylineStateForDeletedItem(gi);
                delete gi;
            }
        }

        if (payload->mergedPts.isEmpty())
            return;

        qgraph::Line* line = new qgraph::Line(_scene, payload->mergedPts.front());
        line->setData(_roleUid, QVariant::fromValue<qulonglong>(payload->mergedUid));

        for (int i = 1; i < payload->mergedPts.size(); ++i)
            line->addPoint(payload->mergedPts[i], _scene);
        line->setNumberingFromLast(payload->numberingFromLast);

        apply_LineWidth_ToItem(line);
        apply_PointSize_ToItem(line);
        apply_NumberSize_ToItem(line);

        line->setData(0, payload->label);
        applyClassColorToItem(line, payload->label);
        line->setZValue(payload->z);
        line->setVisible(true);
        line->updateHandlePosition();
        linkSceneItemToList(line);

        if (Document::Ptr doc = currentDocument())
        {
            doc->isModified = true;
            updateFileListDisplay(doc->filePath);
        }
    };

    std::function<void()> undoFn = [this, payload]()
    {
        if (payload->mergedUid)
        {
            if (QGraphicsItem* gi = findItemByUid(payload->mergedUid))
            {
                _scene->removeItem(gi);
                removeListEntryBySceneItem(gi);
                clearLinePolylineStateForDeletedItem(gi);
                delete gi;
            }
        }

        // Восстанавливаем A и B
        recreateFromBackup(payload->aBackup);
        recreateFromBackup(payload->bBackup);

        if (Document::Ptr doc = currentDocument())
        {
            doc->isModified = true;
            updateFileListDisplay(doc->filePath);
        }
    };

    doc->_undoStack->push(new LambdaCommand(redoFn, undoFn, u8"Объединить линии"));
    return true;
}

bool MainWindow::performSplitLineByEdge(qgraph::Line* line, const QPointF& scenePos)
{
    Document::Ptr doc = currentDocument();
    if (!doc || !doc->_undoStack || !line)
        return false;

    ShapeBackup before = makeBackupFromItem(line);
    const int n = before.points.size();
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
        const QPointF& firstPt = before.points[i];
        const QPointF& secondPt = before.points[i + 1];
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

    struct Payload
    {
        ShapeBackup oldBackup;
        QVector<QPointF> pts1;
        QVector<QPointF> pts2;
        QString label;
        qreal zValue = 0.0;
        bool visible = true;
        bool numberingFromLast = false;
        qulonglong uid1 = 0;
        qulonglong uid2 = 0;
    };

    std::shared_ptr<Payload> payload = std::make_shared<Payload>();
    payload->oldBackup = before;
    payload->label = before.className;
    payload->zValue = before.zValue;
    payload->visible = before.visible;
    payload->numberingFromLast = before.numberingFromLast;

    payload->uid1 = _uidCounter++;
    payload->uid2 = _uidCounter++;

    payload->pts1.reserve(bestIdx + 1);
    for (int i = 0; i <= bestIdx; ++i)
        payload->pts1.push_back(before.points[i]);

    payload->pts2.reserve(n - (bestIdx + 1));
    for (int i = bestIdx + 1; i < n; ++i)
        payload->pts2.push_back(before.points[i]);

    std::function<void()> redoFn = [this, payload]()
    {
        // Удалить старую линию
        if (payload->oldBackup.uid)
        {
            if (QGraphicsItem* gi = findItemByUid(payload->oldBackup.uid))
            {
                _scene->removeItem(gi);
                removeListEntryBySceneItem(gi);
                clearLinePolylineStateForDeletedItem(gi);
                delete gi;
            }
        }

        std::function<void(const QVector<QPointF>&, qulonglong)> makeLine =
            [this, payload](const QVector<QPointF>& pts, qulonglong uid)
        {
            if (pts.size() < 2) return;

            qgraph::Line* line = new qgraph::Line(_scene, pts.front());
            line->setData(_roleUid, QVariant::fromValue<qulonglong>(uid));
            for (int i = 1; i < pts.size(); ++i)
                line->addPoint(pts[i], _scene);

            line->setNumberingFromLast(payload->numberingFromLast);

            apply_LineWidth_ToItem(line);
            apply_PointSize_ToItem(line);
            apply_NumberSize_ToItem(line);

            line->setData(0, payload->label);
            applyClassColorToItem(line, payload->label);
            line->setZValue(payload->zValue);
            line->setVisible(payload->visible);
            line->updateHandlePosition();
            linkSceneItemToList(line);
        };

        makeLine(payload->pts1, payload->uid1);
        makeLine(payload->pts2, payload->uid2);

        if (Document::Ptr doc = currentDocument())
        {
            doc->isModified = true;
            updateFileListDisplay(doc->filePath);
        }
    };

    std::function<void()> undoFn = [this, payload]()
    {
        // Удалить две новые линии
        for (qulonglong uid : {payload->uid1, payload->uid2})
        {
            if (!uid) continue;
            if (QGraphicsItem* gi = findItemByUid(uid))
            {
                _scene->removeItem(gi);
                removeListEntryBySceneItem(gi);
                clearLinePolylineStateForDeletedItem(gi);
                delete gi;
            }
        }

        // Восстановить старую
        recreateFromBackup(payload->oldBackup);

        if (Document::Ptr doc = currentDocument())
        {
            doc->isModified = true;
            updateFileListDisplay(doc->filePath);
        }
    };

    doc->_undoStack->push(new LambdaCommand(redoFn, undoFn, u8"Разрыв линию"));
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
    Q_UNUSED(mods);
    Q_UNUSED(graphView);

    _isInDrawingMode = true;
    setSceneItemsMovable(false);
    updateModeLabel();
    _startPoint = scenePos;

    if (!_polyline)
    {
        _resumeEditing = false;
        _resumeUid = 0;
        // Если полилиния еще не создана, создаем объект
        _polyline = new qgraph::Polyline(_scene, _startPoint);
        ensureUid(_polyline);
        apply_LineWidth_ToItem(_polyline);
        apply_PointSize_ToItem(_polyline);
        apply_NumberSize_ToItem(_polyline);

        _polyline->setFlag(QGraphicsItem::ItemIsMovable, false);
        _polyline->setSelected(true);
        _polyline->setFocus();
        _polyline->updatePointNumbers();

        ShapeBackup b0 = makeBackupFromItem(_polyline);
        pushAdoptExistingShapeCommand(_polyline, b0, u8"Узел полилинии");
        _polyline->setModificationCallback([this]()
        {
            if (_drawingPolyline && _polyline && _polyline->isClosed())
            {
                _drawingPolyline = false;
                updateModeLabel();

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
                            // _polyline->setData(0, selectedClass);
                            // applyClassColorToItem(_polyline, selectedClass);
                            // linkSceneItemToList(_polyline);

                            // ShapeBackup b = makeBackupFromItem(_polyline);
                            // pushAdoptExistingShapeCommand(_polyline, b, u8"Добавление полилинии"));
                            const qulonglong uid = ensureUid(_polyline);
                            const QString cls = selectedClass;

                            _drawingPolyline = false;
                            updateModeLabel();

                            std::function<void()> redoFn = [this, uid, cls]()
                            {
                                qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(findItemByUid(uid));
                                if (!polyline)
                                    return;

                                polyline->setClosed(true, false);
                                polyline->setFlag(QGraphicsItem::ItemIsMovable, true);

                                polyline->setData(0, cls);
                                applyClassColorToItem(polyline, cls);
                                linkSceneItemToList(polyline);

                                if (_polyline && ensureUid(_polyline) == uid)
                                    _polyline = nullptr;

                                _drawingPolyline = false;
                                _resumeEditing = false;
                                _resumeUid = 0;
                                updateModeLabel();

                                if (Document::Ptr doc = currentDocument())
                                {
                                    doc->isModified = true;
                                    updateFileListDisplay(doc->filePath);
                                }
                            };

                            std::function<void()> undoFn = [this, uid]()
                            {
                                qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(findItemByUid(uid));
                                if (!polyline)
                                    return;

                                polyline->setClosed(false, false);
                                polyline->setFlag(QGraphicsItem::ItemIsMovable, false);

                                _drawingPolyline = true;
                                _polyline = polyline;
                                updateModeLabel();

                                _resumeEditing = true;
                                _resumeUid = uid;

                                polyline->setSelected(true);
                                polyline->setFocus();

                                removeListEntryBySceneItem(polyline);

                                if (Document::Ptr doc = currentDocument())
                                {
                                    doc->isModified = true;
                                    updateFileListDisplay(doc->filePath);
                                }

                                raiseAllHandlesToTop();
                            };
                            if (QUndoStack* stack = activeUndoStack())
                                stack->push(new LambdaCommand(redoFn, undoFn, u8"Добавление полилинии"));
                            else
                                redoFn();
                        }
                    }
                }
                else
                {
                    const QString cls = _polyline->data(0).toString();
                    const qulonglong uid = ensureUid(_polyline);
                    _drawingPolyline = false;
                    updateModeLabel();

                    std::function<void()> redoFn = [this, uid, cls]()
                    {
                        qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(findItemByUid(uid));
                        if (!polyline)
                            return;

                        polyline->setClosed(true, false);
                        polyline->setFlag(QGraphicsItem::ItemIsMovable, true);

                        if (!cls.isEmpty())
                        {
                            polyline->setData(0, cls);
                            applyClassColorToItem(polyline, cls);
                        }
                        linkSceneItemToList(polyline);

                        if (_polyline && ensureUid(_polyline) == uid)
                            _polyline = nullptr;

                        _drawingPolyline = false;
                        _resumeEditing = false;
                        _resumeUid = 0;
                        updateModeLabel();

                        if (Document::Ptr doc = currentDocument())
                        {
                            doc->isModified = true;
                            updateFileListDisplay(doc->filePath);
                        }
                        raiseAllHandlesToTop();
                    };

                    std::function<void()> undoFn = [this, uid]()
                    {
                        qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(findItemByUid(uid));
                        if (!polyline)
                            return;

                        polyline->setClosed(false, false);
                        polyline->setFlag(QGraphicsItem::ItemIsMovable, false);

                        _drawingPolyline = true;
                        _polyline = polyline;
                        updateModeLabel();

                        _resumeEditing = true;
                        _resumeUid = uid;

                        polyline->setSelected(true);
                        polyline->setFocus();

                        removeListEntryBySceneItem(polyline);

                        if (Document::Ptr doc = currentDocument())
                        {
                            doc->isModified = true;
                            updateFileListDisplay(doc->filePath);
                        }
                        raiseAllHandlesToTop();
                    };

                    if (QUndoStack* stack = activeUndoStack())
                        stack->push(new LambdaCommand(redoFn, undoFn, u8"Добавление полилинии"));
                    else
                        redoFn();
                }


                _polyline = nullptr;

                if (Document::Ptr doc = currentDocument())
                {
                    doc->isModified = true;
                    updateFileListDisplay(doc->filePath);
                }
                raiseAllHandlesToTop();
            }
        });
    }
    //_polyline->addPoint(_startPoint, _scene);
    const QPointF p = _startPoint;

    if (!_polyline->_circles.isEmpty() &&
        QLineF(_polyline->_circles.last()->scenePos(), p).length() < 0.1)
    {
        // Ничего
    }
    else if (QUndoStack* stack = activeUndoStack())
    {
        struct Payload
        {
            qulonglong uid = 0;
            QPointF    pt;
            bool       didAdd = false;
        };

        std::shared_ptr<Payload> payload = std::make_shared<Payload>();
        payload->uid = ensureUid(_polyline);
        payload->pt  = p;

        std::function<void()> redoFn = [this, payload]()
        {
            qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(findItemByUid(payload->uid));
            if (!polyline)
                return;

            const int before = polyline->_circles.size();
            polyline->addPoint(payload->pt, _scene);

            polyline->setFlag(QGraphicsItem::ItemIsMovable, false);
            polyline->setSelected(true);
            polyline->setFocus();
            polyline->updatePointNumbers();

            payload->didAdd = (polyline->_circles.size() > before);

            if (Document::Ptr doc = currentDocument())
            {
                doc->isModified = true;
                updateFileListDisplay(doc->filePath);
            }
        };

        std::function<void()> undoFn = [this, payload]()
        {
            qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(findItemByUid(payload->uid));
            if (!polyline)
                return;

            if (payload->didAdd)
                polyline->removeLastPointForce(true);

            if (Document::Ptr doc = currentDocument())
            {
                doc->isModified = true;
                updateFileListDisplay(doc->filePath);
            }
        };

        stack->push(new LambdaCommand(redoFn, undoFn, u8"Узел полилинии"));
    }

    // Закрытие полилинии по Ctrl
    if (qgraph::Polyline::globalCloseMode() == qgraph::Polyline::CloseMode::CtrlModifier
        && (mods & Qt::ControlModifier))
    {
        //_polyline->closePolyline();
        if (_polyline && !_polyline->isClosed())
        {
            _polyline->closePolyline();
        }

    }
    apply_PointSize_ToItem(_polyline);
    raiseAllHandlesToTop();
}

void MainWindow::handleLineLmbClick(const QPointF& scenePos, Qt::KeyboardModifiers mods, GraphicsView* graphView)
{
    Q_UNUSED(mods);
    Q_UNUSED(graphView);

     _isInDrawingMode = true;
    setSceneItemsMovable(false);
    updateModeLabel();
    _startPoint = scenePos;
    if (!_line)
    {
        _resumeEditing = false;
        _resumeUid = 0;
        // Если линия еще не создана, создаем объект
        _line = new qgraph::Line(_scene, _startPoint);
        ensureUid(_line);
        apply_LineWidth_ToItem(_line);
        apply_PointSize_ToItem(_line);
        apply_NumberSize_ToItem(_line);
        _line->setSelected(true);
        _line->setFocus();
        _line->updatePointNumbers();
        ShapeBackup b0 = makeBackupFromItem(_line);
        pushAdoptExistingShapeCommand(_line, b0, u8"Узел линии");
        _line->setModificationCallback([this]()
        {
            if (_drawingLine && _line && _line->isClosed())
            {
                _drawingLine = false;
                updateModeLabel();

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
                            // _line->setData(0, selectedClass);
                            // applyClassColorToItem(_line, selectedClass);
                            // linkSceneItemToList(_line);

                            // ShapeBackup b = makeBackupFromItem(_line);
                            // pushAdoptExistingShapeCommand(_line, b, u8"Добавление линии"));
                            const qulonglong uid = ensureUid(_line);
                            const QString cls = selectedClass;

                            _drawingLine = false;
                            updateModeLabel();

                            std::function<void()> redoFn = [this, uid, cls]()
                            {
                                qgraph::Line* line = dynamic_cast<qgraph::Line*>(findItemByUid(uid));
                                if (!line) return;

                                line->setClosed(true, false);
                                line->setFlag(QGraphicsItem::ItemIsMovable, true);
                                line->setData(0, cls);
                                applyClassColorToItem(line, cls);
                                linkSceneItemToList(line);

                                if (_line && ensureUid(_line) == uid)
                                    _line = nullptr;

                                _drawingLine = false;
                                _resumeEditing = false;
                                _resumeUid = 0;
                                updateModeLabel();

                                if (Document::Ptr doc = currentDocument())
                                {
                                    doc->isModified = true;
                                    updateFileListDisplay(doc->filePath);
                                }
                                raiseAllHandlesToTop();
                            };

                            std::function<void()> undoFn = [this, uid]()
                            {
                                qgraph::Line* line = dynamic_cast<qgraph::Line*>(findItemByUid(uid));
                                if (!line) return;

                                line->setClosed(false, false);
                                line->setFlag(QGraphicsItem::ItemIsMovable, false);

                                _drawingLine = true;
                                _line = line;
                                updateModeLabel();

                                _resumeEditing = true;
                                _resumeUid = uid;

                                line->setSelected(true);
                                line->setFocus();

                                removeListEntryBySceneItem(line);

                                if (Document::Ptr doc = currentDocument())
                                {
                                    doc->isModified = true;
                                    updateFileListDisplay(doc->filePath);
                                }
                                raiseAllHandlesToTop();
                            };

                            if (QUndoStack* stack = activeUndoStack())
                                stack->push(new LambdaCommand(redoFn, undoFn, u8"Добавление линии"));
                            else
                                redoFn();

                        }
                    }
                }
                else
                {
                    const qulonglong uid = ensureUid(_line);
                    const QString cls = _line->data(0).toString();

                    _drawingLine = false;
                    updateModeLabel();

                    std::function<void()> redoFn = [this, uid, cls]()
                    {
                        qgraph::Line* line = dynamic_cast<qgraph::Line*>(findItemByUid(uid));
                        if (!line) return;

                        line->setClosed(true, false);
                        line->setFlag(QGraphicsItem::ItemIsMovable, true);

                        if (!cls.isEmpty())
                        {
                            line->setData(0, cls);
                            applyClassColorToItem(line, cls);
                        }
                        linkSceneItemToList(line);

                        if (_line && ensureUid(_line) == uid)
                            _line = nullptr;

                        _drawingLine = false;
                        _resumeEditing = false;
                        _resumeUid = 0;
                        updateModeLabel();

                        if (Document::Ptr doc = currentDocument())
                        {
                            doc->isModified = true;
                            updateFileListDisplay(doc->filePath);
                        }
                        raiseAllHandlesToTop();
                    };

                    std::function<void()> undoFn = [this, uid]()
                    {
                        qgraph::Line* line = dynamic_cast<qgraph::Line*>(findItemByUid(uid));
                        if (!line) return;

                        line->setClosed(false, false);
                        line->setFlag(QGraphicsItem::ItemIsMovable, false);

                        _drawingLine = true;
                        _line = line;
                        updateModeLabel();

                        _resumeEditing = true;
                        _resumeUid = uid;

                        line->setSelected(true);
                        line->setFocus();

                        removeListEntryBySceneItem(line);

                        if (Document::Ptr doc = currentDocument())
                        {
                            doc->isModified = true;
                            updateFileListDisplay(doc->filePath);
                        }
                        raiseAllHandlesToTop();
                    };

                    if (QUndoStack* stack = activeUndoStack())
                        stack->push(new LambdaCommand(redoFn, undoFn, u8"Добавление линии"));
                    else
                        redoFn();
                }

                _line = nullptr;

                if (Document::Ptr doc = currentDocument())
                {
                    doc->isModified = true;
                    updateFileListDisplay(doc->filePath);
                }
                raiseAllHandlesToTop();
            }
        });
    }
    //_line->addPoint(_startPoint, _scene);
    const QPointF p = _startPoint;
    if (!_line->_circles.isEmpty() &&
        QLineF(_line->_circles.last()->scenePos(), p).length() < 0.1)
    {
        // Ничего
    }
    else if (QUndoStack* stack = activeUndoStack())
    {
        struct Payload
        {
            qulonglong uid = {0};
            QPointF    pt;
            bool       didAdd = {false};
        };

        std::shared_ptr<Payload> payload = std::make_shared<Payload>();
        payload->uid = ensureUid(_line);
        payload->pt  = p;

        std::function<void()> redoFn = [this, payload]()
        {
            qgraph::Line* line = dynamic_cast<qgraph::Line*>(findItemByUid(payload->uid));
            if (!line) return;

            const int before = line->_circles.count();
            line->addPoint(payload->pt, _scene);

            line->setSelected(true);
            line->setFocus();
            line->updatePointNumbers();

            payload->didAdd = (line->_circles.size() > before);

            if (Document::Ptr doc = currentDocument())
            {
                doc->isModified = true;
                updateFileListDisplay(doc->filePath);
            }
        };

        std::function<void()> undoFn = [this, payload]()
        {
            qgraph::Line* line = dynamic_cast<qgraph::Line*>(findItemByUid(payload->uid));
            if (!line) return;

            if (payload->didAdd)
                line->removeLastPointForce(true);

            if (Document::Ptr doc = currentDocument())
            {
                doc->isModified = true;
                updateFileListDisplay(doc->filePath);
            }
        };

        stack->push(new LambdaCommand(redoFn, undoFn, u8"Узел линии"));
    }

    // Завершение линии по Ctrl
    if (qgraph::Line::globalCloseMode() == qgraph::Line::CloseMode::CtrlModifier &&
        (mods & Qt::ControlModifier))
    {
        _line->closeLine();
    }
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

    const QList<QListWidgetItem*> selected = ui->polygonList->selectedItems();

    // Перемещение вверх/вниз делаем только для одной выбранной фигуры.
    // Удаление при этом может работать для нескольких.
    if (selected.size() != 1)
        return;

    QListWidgetItem* currentItem = selected.first();
    QGraphicsItem* sceneItem = sceneItemFromListItem(currentItem);

    if (!sceneItem)
        return;

    const int fromRow = ui->polygonList->row(currentItem);
    const int toRow = fromRow + direction;

    if (fromRow < 0 || toRow < 0 || toRow >= ui->polygonList->count())
        return;

    const qulonglong uid = ensureUid(sceneItem);

    std::function<void()> redoFn = [this, uid, toRow]()
    {
        const int currentRow = polygonListRowByUid(uid);
        if (currentRow < 0)
            return;

        movePolygonListRow(currentRow, toRow);

        if (Document::Ptr doc = currentDocument())
        {
            doc->isModified = true;
            updateFileListDisplay(doc->filePath);
        }
    };

    std::function<void()> undoFn = [this, uid, fromRow]()
    {
        const int currentRow = polygonListRowByUid(uid);
        if (currentRow < 0)
            return;

        movePolygonListRow(currentRow, fromRow);

        if (Document::Ptr doc = currentDocument())
        {
            doc->isModified = true;
            updateFileListDisplay(doc->filePath);
        }
    };

    if (doc->_undoStack)
        doc->_undoStack->push(new LambdaCommand(redoFn, undoFn, u8"Перемещение фигуры в списке"));
    else
        redoFn();
}

void MainWindow::movePolygonListRow(int fromRow, int toRow)
{
    if (!ui || !ui->polygonList)
        return;

    const int count = ui->polygonList->count();

    if (fromRow < 0 || fromRow >= count)
        return;

    if (toRow < 0 || toRow >= count)
        return;

    if (fromRow == toRow)
        return;

    QListWidgetItem* movedItem = nullptr;

    {
        QSignalBlocker block(ui->polygonList);

        movedItem = ui->polygonList->takeItem(fromRow);
        if (!movedItem)
            return;

        ui->polygonList->insertItem(toRow, movedItem);

        ui->polygonList->clearSelection();
        movedItem->setSelected(true);
        ui->polygonList->setCurrentItem(movedItem);
    }

    refreshShapeListOrderRole();

    onPolygonListSelectionChanged();
    updateShapeListButtons();

    if (_scene)
        _scene->update();
}

int MainWindow::polygonListRowByUid(qulonglong uid) const
{
    if (!ui || !ui->polygonList || uid == 0)
        return -1;

    for (int i = 0; i < ui->polygonList->count(); ++i)
    {
        QListWidgetItem* listItem = ui->polygonList->item(i);
        QGraphicsItem* sceneItem = sceneItemFromListItem(listItem);

        if (!sceneItem)
            continue;

        if (sceneItem->data(_roleUid).toULongLong() == uid)
            return i;
    }

    return -1;
}

void MainWindow::updateShapeListButtons()
{
    if (!ui || !ui->polygonList)
        return;

    const QList<QListWidgetItem*> selected = ui->polygonList->selectedItems();

    int row = -1;
    if (selected.size() == 1)
        row = ui->polygonList->row(selected.first());

    const int count = ui->polygonList->count();

    ui->btnMoveShapeUp->setEnabled(row > 0);
    ui->btnMoveShapeDown->setEnabled(row >= 0 && row < count - 1);

    ui->btnDelete->setEnabled(!selected.isEmpty());
}

void MainWindow::refreshShapeListOrderRole()
{
    if (!ui || !ui->polygonList)
        return;

    for (int i = 0; i < ui->polygonList->count(); ++i)
    {
        QListWidgetItem* listItem = ui->polygonList->item(i);
        QGraphicsItem* sceneItem = sceneItemFromListItem(listItem);

        if (!sceneItem)
            continue;

        sceneItem->setData(_roleListOrder, i);
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

    if (doc == currentDocument() && ui && ui->polygonList)
    {
        for (int i = 0; i < ui->polygonList->count(); ++i)
        {
            QListWidgetItem* listItem = ui->polygonList->item(i);
            QGraphicsItem* sceneItem = sceneItemFromListItem(listItem);

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

    std::sort(result.begin(), result.end(),
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
