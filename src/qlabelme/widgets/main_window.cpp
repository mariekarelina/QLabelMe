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

#include "qgraphics2/circle.h"
#include "qgraphics2/rectangle.h"
#include "qgraphics2/polyline.h"
#include "qgraphics2/square.h"
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
#include <QSplitter>
#include <QTextEdit>
#include <QListWidget>


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

//    if (_ultraHD)
//        ui->toolBar->setIconSize({48, 48});
//    else
//        ui->toolBar->setIconSize({32, 32});

//    enableButtons(false);
//    disableAdminMode();




    ui->graphView->setScene(_scene);
    _videoRect = new qgraph::VideoRect(_scene);

    _labelConnectStatus = new QLabel(u8"Нет подключения", this);
    ui->statusBar->addWidget(_labelConnectStatus);

    QString vers = u8"Версия: %1 (gitrev: %2)";
    vers = vers.arg(VERSION_PROJECT).arg(GIT_REVISION);
    ui->statusBar->addPermanentWidget(new QLabel(vers, this));



    QString fileName = "/home/marie/фон.png";
    QPixmap pix(fileName);
    _videoRect->setPixmap(pix);

    // Масштабируем фон под размер окна
    pix = pix.scaled(ui->graphView->size(), Qt::KeepAspectRatioByExpanding);
    // Устанавливаем изображение фоном для сцены
    ui->graphView->setBackgroundBrush(pix);

/*

    // Создаем сплиттер
    QSplitter* splitter = new QSplitter(this);
    QSplitter* splitter2 = new QSplitter(this);
    QSplitter* splitter3 = new QSplitter(this);

    splitter->setOrientation(Qt::Horizontal);
    splitter3->setOrientation(Qt::Horizontal);
    splitter2->setOrientation(Qt::Vertical);


    splitter->addWidget(ui->graphView);



    //QListWidget* listWidget1 = new QListWidget(this);
    _listWidget1->addItem("Элемент 1");
    _listWidget1->addItem("Элемент 2");

    //QListWidget* listWidget2 = new QListWidget(this);
    _listWidget2->addItem("Элемент 1");
    _listWidget2->addItem("Элемент 2");
    _listWidget2->addItem("Элемент 3");

    //QListWidget* listWidget3 = new QListWidget(this);
    _listWidget3->addItem("Элемент 1");
    _listWidget3->addItem("Элемент 2");
    _listWidget3->addItem("Элемент 3");
    _listWidget3->addItem("Элемент 4");


    splitter2->addWidget(_listWidget1);
    splitter2->addWidget(_listWidget2);
    splitter2->addWidget(_listWidget3);


    splitter->addWidget(splitter2);


    // Можно настроить размеры областей
    splitter->setStretchFactor(0, 1);       // graphView занимает больше места
    splitter->setStretchFactor(1, 0);       // Лог минимально растягивается


    QVBoxLayout* layout = new QVBoxLayout(ui->centralwidget); // Создаем вертикальный макет
    // Выбираем размещение
    layout->setContentsMargins(0, 50, 0, 0); // Убираем отступы
    layout->setSpacing(0);                  // Уменьшаем расстояния между элементами
    layout->addWidget(splitter); // Добавляем  QSplitter



*/





//    QHBoxLayout *labelLayout = new QHBoxLayout;
//    _selectModeButton = new QToolButton;
//    _selectModeButton->setText(tr("Select"));
//    _selectModeButton->setCheckable(true);
//    _selectModeButton->setChecked(true);

//    labelLayout->addWidget(label);
//    labelLayout->addWidget(_selectModeButton);

//    connect(_selectModeButton, &QAbstractButton::toggled, this, &MainWindow::togglePointerMode);


    // Загружаем файлы из папки при старте
    loadFilesFromFolder("/home/marie/фото тест");  // Укажите нужный путь

    connect(ui->listWidget_9, &QListWidget::currentItemChanged,
            this, &MainWindow::on_listWidget_9_currentItemChanged);



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


    //QPointF scenePos = graphView->mapToScene(mouseEvent->pos());



    if (_drawingCircle)
    {
        //        qgraph::Circle* circle = new qgraph::Circle(_scene, scenePos);
        //        _scene->addItem(circle);
        if (!_isDrawingCircle)
        {
            _startPoint = graphView->mapToScene(mouseEvent->pos());

            if (!_currCircle)
            {
                // Создаем временный круг с радиусом 0
                _currCircle = new QGraphicsEllipseItem();
                _currCircle->setPen(QPen(Qt::green, 2));  // Голубая граница
                _currCircle->setBrush(Qt::transparent); // Прозрачный фон
                _scene->addItem(_currCircle);
            }
            // Задаем позицию и начальный размер
            _currCircle->setRect(QRectF(_startPoint, QSizeF(1, 1)));
            _isDrawingCircle = true;
        }
        else if (_currCircle)
        {
            _isDrawingCircle = false;
        }
        _drawingCircle = false;
    }

    else if (_drawingLine)
    {
//        polyline = new qgraph::Polyline(_scene);
//        _scene->addItem(polyline);
        _startPoint = graphView->mapToScene(mouseEvent->pos());

        if (!_polyline)
        {
            // Если полилиния еще не создана, создаем объект
            _polyline = new qgraph::Polyline(_scene, _startPoint);
        }

        if (mouseEvent->modifiers() & Qt::ControlModifier)
        {
            _polyline->addPoint(_startPoint, _scene);
            _polyline->closePolyline();
            // Завершаем рисование полилинии при зажатом Ctrl
            _drawingLine = false;
            _polyline = nullptr; // Сбрасываем указатель
        }
        else
        {
            // Добавляем новую точку в полилинию
            _polyline->addPoint(_startPoint, _scene);
        }

        //_drawingLine = false;
    }

    else if (_drawingRectangle)
    {
//        qgraph::Rectangle* rectangle = new qgraph::Rectangle(_scene);
//        _scene->addItem(rectangle);
//        rectangle->setFrameScale(1);
        if (!_isDrawingRectangle)
        {
            //QPointF startPoint;
            _startPoint = graphView->mapToScene(mouseEvent->pos()); // Сохраняем начальную точку

            QPointF startPoint2  = _startPoint;
            startPoint2.rx() += 2;
            startPoint2.ry() += 2;

            if (!_currRectangle)
            {
                _currRectangle = new QGraphicsRectItem();
                _currRectangle->setPen(QPen(Qt::blue, 2));  // Установим границу
                _currRectangle->setBrush(QBrush(Qt::transparent)); // По умолчанию — прозрачный
                _scene->addItem(_currRectangle); // Добавляем объект на сцену
            }
            //_currRectangle->setRect(QRectF(_startPoint, _startPoint)); // Начальный размер равен нулю
            _currRectangle->setRect(QRectF(_startPoint, startPoint2));
            _isDrawingRectangle = true; // Включаем флаг рисования
        }
        else if (_currRectangle)
        {
            _isDrawingRectangle = false;
        }
        _drawingRectangle = false;
    }

    // Удаление и добавление точек на полилинию
//    if (mouseEvent->modifiers() & Qt::ControlModifier && !_drawingLine)
//    {
//        QPointF mousePos = graphView->mapToScene(mouseEvent->pos());
//        // Проверяем, был ли клик на полилинии
//        QGraphicsItem* clickedItem = graphView->itemAt(mouseEvent->pos());
//        if (auto* polyline = dynamic_cast<qgraph::Polyline*>(clickedItem)) {
//            // Если клик был на точке
//            for (DragCircle* _circle : polyline->_circles) {
//                if (QLineF(_circle->pos(), polyline->mapFromScene(mousePos)).length() < 10.0) {
//                    polyline->removePoint(mousePos);  // Удаляем точку
//                    return;
//                }
//            }

//            // Если клик на сегменте линии
//            polyline->insertPoint(mousePos);  // Добавляем новую точку
//        }
//    }

    //graphView->mousePressEvent(mouseEvent);
}

void MainWindow::graphicsView_mouseMoveEvent(QMouseEvent* mouseEvent, GraphicsView* graphView)
{
//    if (mouseEvent->modifiers() & Qt::ShiftModifier && mouseEvent->buttons() & Qt::LeftButton)
//       {
//           QGraphicsScene* scene = graphView->scene();
//           if (!scene)
//               return;

//           QPointF delta = graphView->mapToScene(mouseEvent->pos())
//                           - graphView->mapToScene(_lastMousePos);

//           foreach (QGraphicsItem* item, scene->items())
//           {
//               item->moveBy(delta.x(), delta.y());
//           }
//           _lastMousePos = mouseEvent->pos();
//       }

    if (mouseEvent->modifiers() & Qt::ShiftModifier && mouseEvent->buttons() & Qt::LeftButton)
    {
        //graphView->setDragMode(QGraphicsView::ScrollHandDrag);
//                                _selectModeButton->isChecked()
//                                  ? QGraphicsView::RubberBandDrag
//                                  : QGraphicsView::ScrollHandDrag);
        QGraphicsScene* scene = graphView->scene();
        if (!scene)
            return;

        QPointF delta = graphView->mapToScene(mouseEvent->pos())
                        - graphView->mapToScene(_lastMousePos);

        foreach (QGraphicsItem* item, scene->items())
        {

            // Пропускаем точки-ручки полилинии, так как они перемещаются через itemChange()
            if (dynamic_cast<DragCircle*>(item) && item->parentItem())
                continue;

            // Перемещаем каждый объект в сцене на смещение delta
            item->moveBy(delta.x(), delta.y());

            // Если элемент — это Circle, обновляем его манипулятор _circle
            qgraph::Circle* circle = dynamic_cast<qgraph::Circle*>(item);
            if (circle)
            {
                circle->updateHandlePosition();
            }
            else if (auto* rect = dynamic_cast<qgraph::Rectangle*>(item))
            {
                rect->updateHandlePosition(); // Для прямоугольника
            }
            else if (auto* polyline = dynamic_cast<qgraph::Polyline*>(item))
            {
                polyline->updateHandlePosition(); // Для полилинии
            }
        }

        // Сохраняем текущую позицию мыши
        _lastMousePos = mouseEvent->pos();
    }
//       else
//       {
//           QGraphicsView::mouseMoveEvent(mouseEvent);
//       }

    if (_isDrawingRectangle && _currRectangle)
    {
        // Обновляем размеры прямоугольника, пока пользователь тянет мышь
        QRectF newRect(_startPoint, graphView->mapToScene(mouseEvent->pos()));
        _currRectangle->setRect(newRect.normalized()); // normalizing: приводит координаты к нужному порядку (левый верхний угол — начальный)
    }

    if (_isDrawingCircle && _currCircle)
    {
        // Текущая позиция мыши
        QPointF currentPoint = graphView->mapToScene(mouseEvent->pos());

        // Вычисляем расстояние между центром и текущей точкой (радиус)
        qreal radius = QLineF(_startPoint, currentPoint).length();
        // Создаем квадрат для описания окружности, используя радиус
        QRectF circleRect(_startPoint.x() - radius,
                          _startPoint.y() - radius,
                          radius * 2, radius * 2);
        // Устанавливаем обновленный прямоугольник
        _currCircle->setRect(circleRect);
    }


    if (_drawingLine && _currentLine)
    {
        QPointF scenePos = graphView->mapToScene(mouseEvent->pos());
        _currentLine->setEndHandle(scenePos.x(), scenePos.y()); // Обновление линии
//        QPointF currentPoint = graphView->mapToScene(mouseEvent->pos());

//        // Создаем временный путь с добавлением текущей позиции мыши
//        QPainterPath path;
//        for (const QPointF& p : _polylinePoints)
//        {
//            path.lineTo(p);
//        }
//        path.lineTo(currentPoint); // Добавляем текущую позицию
//        _currPolyline->setPath(path);
    }
}

void MainWindow::graphicsView_mouseReleaseEvent(QMouseEvent* mouseEvent, GraphicsView* graphView)
{
    if (mouseEvent->button() == Qt::LeftButton && _isDrawingRectangle)
    {
        _isDrawingRectangle = false; // Завершаем рисование

        // Завершаем рисование и нормализуем прямоугольник
        //QRectF finalRect = rect().normalized();

        //QRectF finalRect = _currRectangle->rect(); // Получаем размеры временного прямоугольника
        //finalRect.moveTo(_currRectangle->pos());  // Добавляем смещение относительно сцены

        // Получаем финальный прямоугольник и нормализуем его (если размеры отрицательны)
        QRectF finalRect = _currRectangle->sceneBoundingRect();

        // Убираем временный прямоугольник из сцены
        _scene->removeItem(_currRectangle);
        delete _currRectangle;
        _currRectangle = nullptr; // Сбрасываем ссылку, чтобы начать новый прямоугольник
        //delete this; // Удаляем временный объект

        // Проверяем минимальный размер
        if (finalRect.width() < 1 || finalRect.height() < 1) {
            return; // Не создаем объект, если размеры некорректны
        }

        // Создаем объект вашего класса Rectangle
        qgraph::Rectangle* rectangle = new qgraph::Rectangle(_scene);
        rectangle->setRealSceneRect(finalRect); // Устанавливаем координаты и размеры

        //_currRectangle = nullptr; // Сбрасываем ссылку, чтобы начать новый прямоугольник
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

        // Проверяем минимальный радиус (например, 5 единиц)
        if (circleRect.width() < 10 || circleRect.height() < 10) {
            return; // Прерываем, если круг слишком маленький
        }

        // Создаем объект вашего класса Circle
        qgraph::Circle* circle = new qgraph::Circle(_scene, _startPoint);

        // Устанавливаем радиус финального круга
        qreal finalRadius = circleRect.width() / 2;
        circle->setRealRadius(static_cast<int>(finalRadius)); // Используем ваш метод установки радиуса
    }
//    else if (mouseEvent->button() == Qt::LeftButton && _isDrawingPolyline)
//    {
//        _isDrawingPolyline = false;

//        // Удаляем временную линию
//        _scene->removeItem(_currPolyline);
//        delete _currPolyline;
//        _currPolyline = nullptr;

//        // Создаем полилинию вашего класса
//        qgraph::Polyline* polyline = new qgraph::Polyline(_scene);
//        // Устанавливаем позиции конечных точек
//        if (!_polylinePoints.isEmpty())
//        {
//            polyline->_circle1->setPos(_polylinePoints[0]);
//            polyline->_circle2->setPos(_polylinePoints[1]);
//            if (_polylinePoints.size() > 2) polyline->_circle3->setPos(_polylinePoints[2]);
//            if (_polylinePoints.size() > 3) polyline->_circle4->setPos(_polylinePoints[3]);
//        }

//        polyline->updatePath();
//    }

    //graphView->mouseReleaseEvent(mouseEvent);
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
//    ui->graphView->setDragMode(_selectModeButton->isChecked()
//                              ? QGraphicsView::RubberBandDrag
//                              : QGraphicsView::ScrollHandDrag);
//    // Это свойство определяет, позволяет ли представление
//    // взаимодействовать со сценой.
//    ui->graphView->setInteractive(_selectModeButton->isChecked());
}


void MainWindow::on_actOpen_triggered(bool)
{
//    QString fileName = QFileDialog::getOpenFileName(this,
//        tr("Open file"), getenv("HOME"));

    QString fileName = "/home/marie/university/студенческий.jpg";
    //QString fileName = "/home/hkarel/4386x2920.jpg";

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

    _drawingRectangle= true;
    //_drawingLine = false;

//    SquareDrawingItem* sq = new SquareDrawingItem();
//    _scene->addItem(sq);
//    // Устанавливаем начальные и конечные точки линии
//    sq->setTopLeftHandle(0, 0);
//    sq->setBottomRightHandle(100, 100);



}

void MainWindow::on_btnLine_clicked(bool)
{
    _btnLineFlag = true;

    _drawingLine = true;
    _drawingRectangle= false;
}

void MainWindow::on_btnCircle_clicked(bool)
{
    _btnCircleFlag = true;

    _drawingCircle = true;
}


void MainWindow::on_btnTest_clicked(bool)
{

}


void MainWindow::closeEvent(QCloseEvent* event)
{
    // В основном окне приложения метод saveGeometry() нужно вызывать в этой
    // точке, иначе геометрия окна будет сохранятся по разному в разных ОС
    saveGeometry();
    event->accept();
}

void MainWindow::loadFilesFromFolder(const QString &folderPath)
{
    QDir directory(folderPath);
    QStringList files = directory.entryList(QStringList() << "*.*", QDir::Files);

    ui->listWidget_9->clear();  // Очищаем список перед добавлением

    foreach(QString filename, files)
    {
        QString filePath = directory.filePath(filename);
        // Создаем превью (50x50 пикселей)
        QPixmap preview(filePath);
        if (!preview.isNull()) {
            preview = preview.scaled(50, 50, Qt::KeepAspectRatio);

            QListWidgetItem *item = new QListWidgetItem(QIcon(preview), filename);
            item->setData(Qt::UserRole, filePath);

            // Добавляем чекбокс
            item->setCheckState(Qt::Unchecked); // По умолчанию не отмечено
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable); // Разрешаем изменение состояния

            ui->listWidget_9->addItem(item);
        }
//        // Сохраняем полный путь в userRole (можно использовать Qt::UserRole)
//        QListWidgetItem *item = new QListWidgetItem(filename);
//        item->setData(Qt::UserRole, directory.filePath(filename));
//        ui->listWidget_9->addItem(item);
    }
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



void MainWindow::on_listWidget_9_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    Q_UNUSED(previous); // Не используем предыдущий элемент

    if (!current)
        return;

    // Помечаем текущий элемент как просмотренный
    current->setCheckState(Qt::Checked);

    // Получаем полный путь к файлу из userRole
    QString filePath = current->data(Qt::UserRole).toString();

    // Загружаем изображение
    QPixmap pix(filePath);
    if (pix.isNull())
    {
        log_error_m << "Failed to load image:" << filePath;
        return;
    }

    // Масштабируем изображение под размер view (сохраняя пропорции)
    pix = pix.scaled(ui->graphView->size(), Qt::KeepAspectRatioByExpanding);

    // Устанавливаем изображение как фон сцены
    ui->graphView->setBackgroundBrush(pix);

    // Если у вас есть _videoRect, можно также установить изображение в него
    if (_videoRect)
    {
        _videoRect->setPixmap(pix);
    }
}

