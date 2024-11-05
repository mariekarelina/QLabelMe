#include "main_window.h"
#include "ui_main_window.h"
//#include "event_window.h"

#include "shared/defmac.h"
#include "shared/logger/logger.h"
#include "shared/logger/format.h"
#include "shared/config/appl_conf.h"
#include "shared/qt/logger_operators.h"

#include "pproto/commands/base.h"
#include "pproto/commands/pool.h"
#include "pproto/logger_operators.h"

#include "qgraphics/circle.h"
#include "qgraphics/rectangle.h"
#include "qgraphics/square.h"
//#include "qgraphics/functions.h"
//#include "qutils/message_box.h"

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
#include <QMouseEvent>
#include <QWheelEvent>
#include <QEvent>
#include <QKeyEvent>
#include <unistd.h>

#define log_error_m   alog::logger().error   (alog_line_location, "MainWin")
#define log_warn_m    alog::logger().warn    (alog_line_location, "MainWin")
#define log_info_m    alog::logger().info    (alog_line_location, "MainWin")
#define log_verbose_m alog::logger().verbose (alog_line_location, "MainWin")
#define log_debug_m   alog::logger().debug   (alog_line_location, "MainWin")
#define log_debug2_m  alog::logger().debug2  (alog_line_location, "MainWin")

QUuidEx MainWindow::_applId;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
    //_socket(new tcp::Socket)
{
    ui->setupUi(this);
    _windowTitle = windowTitle();
    //setWindowTitle(windowTitle() + QString(" (%1)").arg(VERSION_PROJECT));

    QList<QScreen*> screens = QGuiApplication::screens();
    if (!screens.isEmpty())
        _ultraHD = (screens[0]->geometry().width() >= 2560);

    if (_ultraHD)
        ui->toolBar->setIconSize({48, 48});
    else
        ui->toolBar->setIconSize({32, 32});

//    enableButtons(false);
//    disableAdminMode();


    ui->graphView->setScene(&_scene);
    // Кто это?
    _videoRect = new qgraph::VideoRect(&_scene);

    _labelConnectStatus = new QLabel(u8"Нет подключения", this);
    ui->statusBar->addWidget(_labelConnectStatus);

    QString vers = u8"Версия: %1 (gitrev: %2)";
    vers = vers.arg(VERSION_PROJECT).arg(GIT_REVISION);
    ui->statusBar->addPermanentWidget(new QLabel(vers, this));





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

    loadGeometry();
    return true;
}

void MainWindow::deinit()
{
}

// Переопределили функцию, ловим все события
bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    /*if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Backspace)
        {
            _scene.addRect(QRectF(50, 50, 100, 100), QPen(Qt::green));
        }
    }


    QObject::eventFilter(watched, event);
    */
    return 0;
}


void MainWindow::on_actOpen_triggered(bool)
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open file"), getenv("HOME"));

    QPixmap pix(fileName);
    //int width = ui->image->width();
    //int height = ui->image->height();
    //ui->image->setPixmap(pix.scaled(width, height, Qt::KeepAspectRatio));

//    if (!img.isNull())
//    {
//        QPixmap pixmap = QPixmap::fromImage(img);
        _videoRect->setPixmap(pix);
        _videoRect->setFlags(QGraphicsItem::ItemIsMovable);
//    }

    //QGraphicsRectItem *rect = _scene.addRect(QRectF(0, 0, 100, 100));





/*    qgraph::Rectangle* rect = new qgraph::Rectangle(&_scene);

    rect->setRect({0, 0, 100, 100});
    rect->moveBy(100, 100);
    //rect->setFrameScale(float(frameScale) / 100);
    rect->setFrameScale(1.0);
    rect->shapeVisible(true);
    rect->setZLevel(1);
    //rect->circleMoveSignal.connect(CLOSURE(&MainWindow::graphicShapeChange, this));
    rect->setPenColor(QColor(0, 255, 0));
    //rect->setTag(toInt(tag));

    //qgraph::Circle* circle = dynamic_cast<qgraph::Circle*>(item);
    qgraph::Circle* circle = new qgraph::Circle(&_scene);
    circle->setRect({0, 0, 100, 100});
    circle->moveBy(300, 100);
    rect->setFrameScale(1.0);

    circle->shapeVisible(true);
    circle->setZLevel(2);
    circle->setPenColor(QColor(255, 0, 0));
*/


//data::DetectRect tableRect = roiGeometry.table;
//qgraph::Rectangle* table = new qgraph::Rectangle(&_scene);
//createElementRectangle(table, tableRect,
//                       QColor(0, 255, 0), ElementTag::Table);



}

void MainWindow::on_actClose_triggered(bool)
{
    close();
}

void MainWindow::on_actionCreate_rectangle_triggered()
{
//    ui->graphView->setMouseTracking(true);
//    qgraph::Rectangle* rect = new qgraph::Rectangle(&_scene);
//    if (!point.isNull())
//    {
//        rect->setRect({qreal(point.x()), qreal(point.y()), 100, 100});
//    }
//    else
//    {
//    }


    _scene.addRect(QRectF(50, 50, 100, 100), QPen(Qt::red));
    qgraph::Rectangle* rect = new qgraph::Rectangle(&_scene);
    rect->setRect({0, 0, 100, 100});
    rect->setRect({0, 0, 100, 100});
    rect->moveBy(100, 100);
    //rect->setFrameScale(float(frameScale) / 100);
    rect->setFrameScale(1.0);
    rect->shapeVisible(true);
    rect->setZLevel(1);
    //rect->circleMoveSignal.connect(CLOSURE(&MainWindow::graphicShapeChange, this));
    rect->setPenColor(QColor(0, 255, 0));
    //rect->setTag(toInt(tag));
    //Ловим координаты курсора
    /*if (event->button() == Qt::LeftButton)
    {
        //QPoint QCursor::pos ();
        qgraph::Rectangle* rect = new qgraph::Rectangle(&_scene);
        rect->setRect({0, 0, 100, 100});
        rect->moveBy(100, 100);
        //rect->setFrameScale(float(frameScale) / 100);
        rect->setFrameScale(1.0);
        rect->shapeVisible(true);
        rect->setZLevel(1);
        //rect->circleMoveSignal.connect(CLOSURE(&MainWindow::graphicShapeChange, this));
        rect->setPenColor(QColor(0, 255, 0));
        //rect->setTag(toInt(tag));
    }*/



}

void MainWindow::on_actionCreate_circle_triggered()
{
    _scene.addRect(QRectF(100, 100, 100, 100), QPen(Qt::green));
    QGraphicsView* graphView = new QGraphicsView;
    GraphicsScene* scene = new GraphicsScene();
    qgraph:: Circle* circ = new qgraph::Circle();
    graphView->setScene(scene);
    graphView->setSceneRect(-300,-300, 300, 300);
    this->setCentralWidget(graphView);
    this->resize(600, 600);
}

void MainWindow::on_actionCreate_line_triggered()
{
    /*QVBoxLayout *layout = new QVBoxLayout(this);
    setLayout(layout);
    // Создаем QGraphicsScene и QGraphicsView
    QGraphicsScene *scene = new QGraphicsScene(this);
    QGraphicsView *view = new QGraphicsView(scene);
    layout->addWidget(view);
    // Создаем полилинию с несколькими точками
    QPolygonF points;
    points << QPointF(50, 50) << QPointF(150, 150) << QPointF(250, 50);
    Polyline *polyline = new Polyline(points);

    // Добавляем полилинию в сцену
    scene->addItem(polyline);

    // Кнопка для добавления точки
    QPushButton *addPointButton = new QPushButton("Добавить точку");
    layout->addWidget(addPointButton);

    connect(addPointButton, &QPushButton::clicked, [polyline]() {
        polyline->addPoint(QPointF(300, 200)); // Добавляем новую точку
    }*/
}


void MainWindow::on_toolButton_clicked()
{
    ZoomIn();
}


void MainWindow::on_toolButton_2_clicked()
{
    ZoomOut();
}



void MainWindow::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        // Начинаем перетаскивание
        dragging = true;
        lastMousePos = event->pos();
    }
}


void MainWindow::mouseMoveEvent(QMouseEvent* event)
{
    if (dragging)
    {
        // Перемещаем сцену на основе перемещения мыши
        QPointF delta = ui->graphView->mapToScene(event->pos()) -
                        ui->graphView->mapToScene(lastMousePos);
        ui->graphView->translate(delta.x(), delta.y());
        // Обновляем последнюю позицию мыши
        lastMousePos = event->pos();
    }
}


void MainWindow::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        // Прекращаем перетаскивание
        dragging = false;
    }
}

void MainWindow::wheelEvent(QWheelEvent* event)
{
    // Получаем позицию курсора мыши в сцене
    QPointF scenePos = ui->graphView->mapToScene(event->pos());
    if (event->angleDelta().y() > 0)
    {
        //ZoomIn();  // При прокрутке вверх - увеличиваем масштаб
        ZoomInPos(scenePos);
    }
    else
    {
        //ZoomOut(); // При прокрутке вниз - уменьшаем масштаб
        ZoomOutPos(scenePos);
    }
    event->accept(); // Указываем, что событие обработано
}


// setMouseTracking() - отслеживание мыши
/*
    Функция Position() определяет положение курсора относительно виджета или элемента,
    который получает событие мыши. Если вы перемещаете виджет в результате события мыши,
    используйте глобальную позицию, возвращаемую функцией globalPosition(), чтобы избежать тряски.
*/


void MainWindow::closeEvent(QCloseEvent* event)
{
    // В основном окне приложения метод saveGeometry() нужно вызывать в этой
    // точке, иначе геометрия окна будет сохранятся по разному в разных ОС
    saveGeometry();
    event->accept();
}

void MainWindow::loadGeometry()
{
    QVector<int> v {10, 10, 800, 600};
    config::base().getValue("windows.main_window.geometry", v);
    setGeometry(v[0], v[1], v[2], v[3]);

    QVector<int> splitterSizes;
    if (config::base().getValue("windows.main_window.splitter_sizes", splitterSizes))
        ui->splitter->setSizes(splitterSizes.toList());

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

    //QList<int> splitterSizes = ui->splitter->sizes();
    //config::base().setValue("windows.main_window.splitter_sizes", splitterSizes.toVector());

//    config::base().setValue("windows.main_window.tab_index", ui->tabWidget->currentIndex());

//    QByteArray ba = ui->tableJournal->horizontalHeader()->saveState().toBase64();
//    config::base().setValue("windows.main_window.event_journal_header", QString::fromLatin1(ba));
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    int w = 200; // ширина каждого прямоугольника
    int h = 100;  // высота каждого прямоугольника


    painter.setBrush(Qt::green);
    painter.drawRect(100, 100, w, h);    // левый верхний
    painter.setBrush(Qt::yellow);
    painter.drawRect(w + 100, 100, w, h);    // правый верхний
    painter.setBrush(Qt::magenta);
    painter.drawRect(100, h + 100, w, h);    // левый нижний
    painter.setBrush(Qt::cyan);
    painter.drawRect(w + 100, h + 100, w, h);    // правый нижний
}


void MainWindow::ZoomIn()
{
    ui->graphView->scale(1.1, 1.1);
}

void MainWindow::ZoomOut()
{
    ui->graphView->scale(1.0 / 1.1, 1.0 / 1.1);
}


void MainWindow::ZoomInPos(QPointF scenePos)
{
    // Получаем текущую позицию центра
    QPointF viewCenter = ui->graphView->mapToScene(ui->graphView->viewport()->rect().center());

    // Увеличиваем масштаб на 10%
    ui->graphView->scale(1.1, 1.1); // Увеличиваем масштаб по обеим осям

    // Вычисляем новое положение центра
    QPointF newViewCenter = ui->graphView->mapToScene(ui->graphView->viewport()->rect().center());
    QPointF offset = scenePos - newViewCenter;

    // Перемещаем вид, чтобы сохранить позицию курсора
    ui->graphView->translate(offset.x(), offset.y());
}

void MainWindow::ZoomOutPos(QPointF scenePos)
{
    ui->graphView->scale(1.0 / 1.1, 1.0 / 1.1);
}





