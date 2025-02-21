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
#include "line.h"
#include "square.h"
#include "circle.h"
//#include "qgraphics/functions.h"
//#include "qutils/message_box.h"
#include <QSlider>

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

using namespace qgraph;

//#include <view.h>

#define log_error_m   alog::logger().error   (alog_line_location, "MainWin")
#define log_warn_m    alog::logger().warn    (alog_line_location, "MainWin")
#define log_info_m    alog::logger().info    (alog_line_location, "MainWin")
#define log_verbose_m alog::logger().verbose (alog_line_location, "MainWin")
#define log_debug_m   alog::logger().debug   (alog_line_location, "MainWin")
#define log_debug2_m  alog::logger().debug2  (alog_line_location, "MainWin")

QUuidEx MainWindow::_applId;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    _scene(new QGraphicsScene(this))
    //_socket(new tcp::Socket)
{
    ui->setupUi(this);
    _windowTitle = windowTitle();

    ui->graphView->init(this);
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




    ui->graphView->setScene(_scene);
    _videoRect = new qgraph::VideoRect(_scene);

    _labelConnectStatus = new QLabel(u8"Нет подключения", this);
    ui->statusBar->addWidget(_labelConnectStatus);

    QString vers = u8"Версия: %1 (gitrev: %2)";
    vers = vers.arg(VERSION_PROJECT).arg(GIT_REVISION);
    ui->statusBar->addPermanentWidget(new QLabel(vers, this));



//    QHBoxLayout *labelLayout = new QHBoxLayout;
//    selectModeButton = new QToolButton;
//    selectModeButton->setText(tr("Select"));
//    selectModeButton->setCheckable(true);
//    selectModeButton->setChecked(true);

//    labelLayout->addWidget(label);
//    labelLayout->addWidget(selectModeButton);

//    connect(selectModeButton, &QAbstractButton::toggled, this, &MainWindow::togglePointerMode);



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

void MainWindow::graphicsView_mousePressEvent(QMouseEvent* mouseEvent, GraphicsView* graphView)
{
    //graphView->setCursor(QCursor(Qt::ClosedHandCursor));
    if (mouseEvent->button() == Qt::LeftButton
        && (mouseEvent->modifiers() & Qt::ShiftModifier))
    {
        // Проверяем, есть ли элемент под курсором мыши
        _draggingItem = graphView->itemAt(mouseEvent->pos());
        // Возвращает указатель
        qgraph::VideoRect* videoRect = dynamic_cast<qgraph::VideoRect*>(_draggingItem);
        if (videoRect)
        {
            // Запоминаем начальную позицию курсора
            _lastMousePos = mouseEvent->pos();
        }
    }

//    else
//    {
//        QGraphicsView::mousePressEvent(mouseEvent);
//    }


    QPointF scenePos = graphView->mapToScene(mouseEvent->pos());


//    qgraph::Rectangle* roi = new qgraph::Rectangle(_scene);
//    roi->setRect({0, 0, 50, 100});



    if (drawingLine)
    {
        if (!currentLine)
        {
            currentLine = new LineDrawingItem();
            _scene->addItem(currentLine);
            currentLine->setStartHandle(scenePos.x(), scenePos.y()); // Устанавливаем первую ручку
        }
        else
        {
            currentLine->setEndHandle(scenePos.x(), scenePos.y()); // Устанавливаем вторую ручку
            currentLine = nullptr; // Завершаем рисование линии
            drawingLine = false;
        }
    }
    else if (drawingSquare)
    {
//        if (!currentSquare)
//        {
//            currentSquare = new SquareDrawingItem();
//            _scene->addItem(currentSquare);
//            currentSquare->setTopLeftHandle(scenePos.x(), scenePos.y()); // Устанавливаем первую ручку
//        }
//        else
//        {
//            currentSquare->setBottomRightHandle(scenePos.x(), scenePos.y()); // Устанавливаем вторую ручку
//            currentSquare = nullptr; // Завершаем рисование квадрата
//            drawingSquare = false;
//        }
        qgraph::Rectangle* roi = new qgraph::Rectangle(_scene);
        roi->setRect({scenePos.x(), scenePos.y(), 50, 100});

        drawingSquare = false;

        //roi->changeSignal.connect(CLOSURE(&MainWindow::graphicShapeChange, this));

    }


    //graphView->mousePressEvent(mouseEvent);
}

void MainWindow::graphicsView_mouseMoveEvent(QMouseEvent* mouseEvent, GraphicsView* graphView)
{
    if (mouseEvent->modifiers() & Qt::ShiftModifier && mouseEvent->buttons() & Qt::LeftButton)
       {
           QGraphicsScene* scene = graphView->scene();
           if (!scene)
               return;

           QPointF delta = graphView->mapToScene(mouseEvent->pos())
                           - graphView->mapToScene(_lastMousePos);

           foreach (QGraphicsItem* item, scene->items())
           {
               item->moveBy(delta.x(), delta.y());
           }
           _lastMousePos = mouseEvent->pos();
       }
//       else
//       {
//           QGraphicsView::mouseMoveEvent(mouseEvent);
//       }


    if (drawingLine && currentLine)
    {
        QPointF scenePos = graphView->mapToScene(mouseEvent->pos());
        currentLine->setEndHandle(scenePos.x(), scenePos.y()); // Обновление линии
    }
    else if (drawingSquare && currentSquare)
    {
        QPointF scenePos = graphView->mapToScene(mouseEvent->pos());
        currentSquare->setBottomRightHandle(scenePos.x(), scenePos.y()); // Обновление квадрата
    }
}

void MainWindow::graphicsView_mouseReleaseEvent(QMouseEvent* mouseEvent, GraphicsView* graphView)
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

void MainWindow::togglePointerMode()
{
//    ui->graphView->setDragMode(selectModeButton->isChecked()
//                              ? QGraphicsView::RubberBandDrag
//                              : QGraphicsView::ScrollHandDrag);
//    // Это свойство определяет, позволяет ли представление
//    // взаимодействовать со сценой.
//    ui->graphView->setInteractive(selectModeButton->isChecked());
}


void MainWindow::on_actOpen_triggered(bool)
{
//    QString fileName = QFileDialog::getOpenFileName(this,
//        tr("Open file"), getenv("HOME"));

    QString fileName = "/home/marie/university/студенческий.jpg";

    QPixmap pix(fileName);
    //int width = ui->image->width();
    //int height = ui->image->height();
    //ui->image->setPixmap(pix.scaled(width, height, Qt::KeepAspectRatio));

    if (pix.isNull()) {
        // Обработка ошибок, если изображение не загружено
        return;
    }

    _videoRect->setPixmap(pix);
    _videoRect->setFlags(QGraphicsItem::ItemIsMovable);

//    QSlider *zoomSlider = new QSlider;
//    zoomSlider->setMinimum(0);
//    zoomSlider->setMaximum(500);
//    zoomSlider->setValue(250);
//    zoomSlider->setTickPosition(QSlider::TicksRight);
//    // Zoom slider layout
//    QVBoxLayout *zoomSliderLayout = new QVBoxLayout;
//    zoomSliderLayout->addWidget(zoomSlider);

//    QGridLayout *topLayout = new QGridLayout;
//    topLayout->addLayout(zoomSliderLayout, 1, 1);
//    setLayout(topLayout);


//data::DetectRect tableRect = roiGeometry.table;
//qgraph::Rectangle* table = new qgraph::Rectangle(&_scene);
//createElementRectangle(table, tableRect,
//                       QColor(0, 255, 0), ElementTag::Table);



}

void MainWindow::on_actClose_triggered(bool)
{
    close();
}

void MainWindow::on_actCreateRectangle_triggered()
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


//    _scene.addRect(QRectF(50, 50, 100, 100), QPen(Qt::red));
//    qgraph::Rectangle* rect = new qgraph::Rectangle(&_scene);
//    rect->setRect({0, 0, 100, 100});
//    rect->setRect({0, 0, 100, 100});
//    rect->moveBy(100, 100);
//    //rect->setFrameScale(float(frameScale) / 100);
//    rect->setFrameScale(1.0);
//    rect->shapeVisible(true);
//    rect->setZLevel(1);
//    //rect->circleMoveSignal.connect(CLOSURE(&MainWindow::graphicShapeChange, this));
//    rect->setPenColor(QColor(0, 255, 0));
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

void MainWindow::on_actCreateCircle_triggered()
{
//    _scene.addRect(QRectF(100, 100, 100, 100), QPen(Qt::green));
//    QGraphicsView* graphView = new QGraphicsView;
//    GraphicsScene* scene = new GraphicsScene();
//    //qgraph:: Circle* circ = new qgraph::Circle();
//    graphView->setScene(scene);
//    graphView->setSceneRect(-300,-300, 300, 300);
//    this->setCentralWidget(graphView);
//    this->resize(600, 600);
}

void MainWindow::on_actCreateLine_triggered()
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


void MainWindow::on_btnRect_clicked(bool)
{
//    qgraph::Rectangle* roi = new qgraph::Rectangle(_scene);
//    roi->setRect({0, 0, 50, 100});

//    //QPointF _scenePos
//    double size = 50; // Размер стороны квадрата
//    _scene.addRect(QRectF(_scenePos.x() - size / 2, _scenePos.y() - size / 2, size, size),
//                   QPen(Qt::red));
    //_btnRectFlag = true;

    drawingSquare = true;
    //drawingLine = false;

//    SquareDrawingItem* sq = new SquareDrawingItem();
//    _scene->addItem(sq);
//    // Устанавливаем начальные и конечные точки линии
//    sq->setTopLeftHandle(0, 0);
//    sq->setBottomRightHandle(100, 100);



}

void MainWindow::on_btnLine_clicked(bool)
{
    _btnLineFlag = true;

    drawingLine = true;
    drawingSquare = false;

//    LineDrawingItem* line = new LineDrawingItem();
//    _scene->addItem(line);
//    // Устанавливаем начальные и конечные точки линии
//    line->setStartHandle(0, 0);
//    line->setEndHandle(100, 100);
}

void MainWindow::on_btnCircle_clicked(bool)
{
    _btnCircleFlag = true;
}


void MainWindow::on_btnTest_clicked(bool)
{
    //_btnLineFlag = true;
}


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
//    if (config::base().getValue("windows.main_window.splitter_sizes", splitterSizes))
//        ui->splitter->setSizes(splitterSizes.toList());

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


//void MainWindow::zoomIn()
//{
//    ui->graphView->scale(1.1, 1.1);
//}

//void MainWindow::zoomOut()
//{
//    ui->graphView->scale(1.0 / 1.1, 1.0 / 1.1);
//}


//void MainWindow::zoomInPos(QPointF scenePos)
//{
//    // Получаем текущую позицию центра
//    QPointF viewCenter = ui->graphView->mapToScene(ui->graphView->viewport()->rect().center());

//    // Увеличиваем масштаб на 10%
//    ui->graphView->scale(1.1, 1.1); // Увеличиваем масштаб по обеим осям

//    // Вычисляем новое положение центра
//    QPointF newViewCenter = ui->graphView->mapToScene(ui->graphView->viewport()->rect().center());
//    QPointF offset = scenePos - newViewCenter;

//    // Перемещаем вид, чтобы сохранить позицию курсора
//    ui->graphView->translate(offset.x(), offset.y());
//}

//void MainWindow::zoomOutPos(QPointF scenePos)
//{
//    ui->graphView->scale(1.0 / 1.1, 1.0 / 1.1);
//}


