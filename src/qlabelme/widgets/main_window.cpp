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
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>


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

    bool ultraHD = false;
    QList<QScreen*> screens = QGuiApplication::screens();
    if (!screens.isEmpty())
        ultraHD = (screens[0]->geometry().width() >= 2560);

    if (ultraHD)
        ui->toolBar->setIconSize({48, 48});
    else
        ui->toolBar->setIconSize({32, 32});

    _windowTitle = windowTitle();

    ui->graphView->init(this);
    //setWindowTitle(windowTitle() + QString(" (%1)").arg(VERSION_PROJECT));

//    enableButtons(false);
//    disableAdminMode();

    ui->graphView->setScene(_scene);
    _videoRect = new qgraph::VideoRect(_scene);

    _labelConnectStatus = new QLabel(u8"Нет подключения", this);
    ui->statusBar->addWidget(_labelConnectStatus);

    // Создаем и настраиваем метки
    _folderPathLabel = new QLabel(this);
    _folderPathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    _folderPathLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    ui->statusBar->addWidget(_folderPathLabel, 1); // Растягиваемый

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


    // Загружаем файлы из папки при старте
    loadFilesFromFolder("/home/marie/тестовые данные QLabelMe/фото");  // Укажите нужный путь
    // Загрузка данных из файла при запуске
    loadPolygonLabelsFromFile("/home/marie/тестовые данные QLabelMe/список классов/список классов.txt"); // Укажите актуальный путь к файлу

    connect(ui->listWidget_FileList, &QListWidget::currentItemChanged,
            this, &MainWindow::fileList_ItemChanged);



    QLabel* pathHeader = new QLabel(this);
    pathHeader->setAlignment(Qt::AlignCenter);
    // Добавляем заголовок над listWidget
    ui->verticalLayout->insertWidget(0, pathHeader);

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
    _rightPanel = ui->splitter2->widget(1); // Или другой индекс, в зависимости от структуры
    // Сохраняем начальные размеры сплиттера
    _savedSplitterSizes = ui->splitter2->sizes();

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
        setSceneItemsMovable(false);
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
        setSceneItemsMovable(false);
        _startPoint = graphView->mapToScene(mouseEvent->pos());
        if (!_polyline)
        {
            // Если полилиния еще не создана, создаем объект
            _polyline = new qgraph::Polyline(_scene, _startPoint);
        }
        _polyline->addPoint(_startPoint, _scene);
    }

    else if (_drawingRectangle)
    {
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

    //graphView->mousePressEvent(mouseEvent);
}

void MainWindow::graphicsView_mouseMoveEvent(QMouseEvent* mouseEvent, GraphicsView* graphView)
{
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

            if (auto* circle = dynamic_cast<qgraph::Circle*>(item))
            {
                circle->updateHandlePosition();
            }
            else if (auto* rect = dynamic_cast<qgraph::Rectangle*>(item))
            {
                rect->updateHandlePosition();
            }
            else if (auto* polyline = dynamic_cast<qgraph::Polyline*>(item))
            {
                polyline->updateHandlePosition();
            }
        }
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
        _currRectangle->setRect(newRect.normalized());
        // normalizing: приводит координаты к нужному порядку (левый верхний угол — начальный)
    }

    else if (_isDrawingCircle && _currCircle)
    {
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

    else if (_isDrawingPolyline && _currPolyline)
    {
        QPointF scenePos = graphView->mapToScene(mouseEvent->pos());
        _currentLine->setEndHandle(scenePos.x(), scenePos.y()); // Обновление линии
    }
}

void MainWindow::graphicsView_mouseReleaseEvent(QMouseEvent* mouseEvent, GraphicsView* graphView)
{
    setSceneItemsMovable(true);
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

        // Получаем список классов из listWidget_PolygonLabel
        QStringList classes;
        for (int i = 0; i < ui->listWidget_PolygonLabel->count(); ++i)
        {
            classes << ui->listWidget_PolygonLabel->item(i)->text();
        }

        // Показываем диалог выбора класса
        SelectionDialog dialog(classes, this);
        if (dialog.exec() == QDialog::Accepted)
        {
            QString selectedClass = dialog.selectedClass();
            if (!selectedClass.isEmpty())
            {
                rectangle->setData(0, selectedClass);
                ui->listWidget_PolygonList->addItem(selectedClass);
            }
        }
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

        // Проверяем минимальный радиус
        if (circleRect.width() < 10 || circleRect.height() < 10)
            return; // Прерываем, если круг слишком маленький


        // Создаем объект класса Circle
        qgraph::Circle* circle = new qgraph::Circle(_scene, _startPoint);
        // Устанавливаем радиус финального круга
        qreal finalRadius = circleRect.width() / 2;
        circle->setRealRadius(static_cast<int>(finalRadius));

        QStringList classes;
        for (int i = 0; i < ui->listWidget_PolygonLabel->count(); ++i)
        {
            classes << ui->listWidget_PolygonLabel->item(i)->text();
        }

        SelectionDialog dialog(classes, this);
        if (dialog.exec() == QDialog::Accepted)
        {
            QString selectedClass = dialog.selectedClass();
            if (!selectedClass.isEmpty())
            {
                circle->setData(0, selectedClass);
                ui->listWidget_PolygonList->addItem(selectedClass);
            }
        }
    }

    else if (mouseEvent->button() == Qt::LeftButton && _drawingLine && _polyline &&
                (mouseEvent->modifiers() & Qt::ControlModifier))
    {
       // Завершаем рисование полилинии при зажатом Ctrl
       _drawingLine = false;
       _polyline->closePolyline();

       // Получаем список классов из listWidget_PolygonLabel
       QStringList classes;
       for (int i = 0; i < ui->listWidget_PolygonLabel->count(); ++i)
       {
           classes << ui->listWidget_PolygonLabel->item(i)->text();
       }

       // Показываем диалог выбора класса
       SelectionDialog dialog(classes, this);
       if (dialog.exec() == QDialog::Accepted)
       {
           QString selectedClass = dialog.selectedClass();
           if (!selectedClass.isEmpty())
           {
               // Устанавливаем класс для полилинии
               _polyline->setData(0, selectedClass);
               ui->listWidget_PolygonList->addItem(selectedClass);
           }
       }
       _polyline = nullptr; // Сбрасываем указатель
    }

    //graphView->mouseReleaseEvent(mouseEvent);
}


void MainWindow::setSceneItemsMovable(bool movable)
{
    for (QGraphicsItem* item : _scene->items())
    {
        item->setFlag(QGraphicsItem::ItemIsMovable, movable);
        //item->setFlag(QGraphicsItem::ItemIsSelectable, movable);
    }
}


bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    return 0;
//    if (event->type() == QEvent::KeyPress)
//    {
//        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
//        if (keyEvent->key() == Qt::Key_Delete)
//        {
//            // Удаление выделенных фигур
//            QList<QGraphicsItem*> selectedItems = _scene->selectedItems();
//            for (QGraphicsItem* item : selectedItems)
//            {
//                _scene->removeItem(item);
//                delete item;
//            }
//            return true;  // Indicate that we processed the event
//        }
//    }
//    return QMainWindow::eventFilter(watched, event);  // Продолжаем обработку остальных событий
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_I)
    {
        resetViewToDefault();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_R && event->modifiers() == Qt::NoModifier)
    {
        toggleRightSplitter();
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
//    QString fileName = QFileDialog::getOpenFileName(this,
//        tr("Open file"), getenv("HOME"));

    QString fileName = "/home/marie/university/студенческий.jpg";
    //QString fileName = "/home/hkarel/4386x2920.jpg";

    QPixmap pix(fileName);
    //int width = ui->image->width();
    //int height = ui->image->height();
    //ui->image->setPixmap(pix.scaled(width, height, Qt::KeepAspectRatio));

    if (pix.isNull())
        // Обработка ошибок, если изображение не загружено
        return;

    _videoRect->setPixmap(pix);
    _videoRect->setFlags(QGraphicsItem::ItemIsMovable);

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

void MainWindow::on_actExit_triggered(bool)
{
    //qApp->quit();
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
    _drawingRectangle= true;
    setSceneItemsMovable(false);
}

void MainWindow::on_btnLine_clicked(bool)
{
    _drawingLine = true;
    setSceneItemsMovable(false);
}

void MainWindow::on_btnCircle_clicked(bool)
{
    _drawingCircle = true;
    setSceneItemsMovable(false);
}


void MainWindow::on_btnTest_clicked(bool)
{

}


void MainWindow::closeEvent(QCloseEvent* event)
{
    // if (!_currentImagePath.isEmpty())
    // {
    //     saveAnnotationToFile(_currentImagePath);
    // }
    // Сохраняем аннотации для всех изображений
    for (auto it = _scenesMap.begin(); it != _scenesMap.end(); ++it)
    {
        const QString& imagePath = it.key();
        QGraphicsScene* scene = it.value();

        // Создаем временную переменную для хранения текущей сцены
        QGraphicsScene* tempScene = _scene;
        _scene = scene; // Устанавливаем текущую сцену для сериализации

        saveAnnotationToFile(imagePath);

        _scene = tempScene; // Восстанавливаем исходную сцену
    }
    // В основном окне приложения метод saveGeometry() нужно вызывать в этой
    // точке, иначе геометрия окна будет сохранятся по разному в разных ОС
    saveGeometry();
    event->accept();
}

void MainWindow::loadFilesFromFolder(const QString &folderPath)
{
    QDir directory(folderPath);
    _currentFolderPath = folderPath; // Сохраняем путь

    // Путь к папке в заголовке окна
    updateWindowTitle();

    //Путь к папке внизу окна
    updateFolderPathDisplay();

    ui->listWidget_FileList->clear();  // Очищаем список перед добавлением
    // Путь к папке над полем FileList
    ui->labelFileListPath->setText(QDir::toNativeSeparators(folderPath));


    QStringList files = directory.entryList(QStringList() << "*.*", QDir::Files);

    // Добавляем файлы
    for (auto filename : files)
    {
        QString filePath = directory.filePath(filename);
        // Создаем превью (50x50 пикселей)
        QPixmap preview(filePath);
        if (!preview.isNull())
        {
            preview = preview.scaled(50, 50, Qt::KeepAspectRatio);

            QListWidgetItem *item = new QListWidgetItem(QIcon(preview), filename);
            // Потом передаем структуру
            item->setData(Qt::UserRole, filePath);

            // Добавляем чекбокс
            item->setCheckState(Qt::Unchecked); // По умолчанию не отмечено
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable); // Разрешаем изменение состояния

            ui->listWidget_FileList->addItem(item);
        }
//        // Сохраняем полный путь в userRole (можно использовать Qt::UserRole)
//        QListWidgetItem *item = new QListWidgetItem(filename);
//        item->setData(Qt::UserRole, directory.filePath(filename));
//        ui->listWidget_9->addItem(item);
    }
}

void MainWindow::loadPolygonLabelsFromFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        log_error_m << "Failed to open file:" << filePath;
        return;
    }

    QTextStream in(&file);
    in.setCodec("UTF-8");
    ui->listWidget_PolygonLabel->clear(); // Очищаем список перед загрузкой

    while (!in.atEnd())
    {
        QString line = in.readLine().trimmed();
        if (!line.isEmpty())
        {
            // Добавляем текст в QListWidget
            ui->listWidget_PolygonLabel->addItem(line);
        }
    }

    file.close();
}


void MainWindow::updatePolygonListForCurrentScene()
{
    ui->listWidget_PolygonList->clear(); // Очищаем текущий список
    if (!_scene) return;

    // Перебираем все элементы на текущей сцене
    for (QGraphicsItem* item : _scene->items())
    {
        // Пропускаем само изображение и временные элементы
        if (item == _videoRect || item == _tempRectItem ||
            item == _tempCircleItem || item == _tempPolyline)
        {
            continue;
        }

        // Получаем класс разметки (если он есть)
        QString className = item->data(0).toString();
        if (!className.isEmpty())
        {
            // Добавляем в список
            QListWidgetItem* listItem = new QListWidgetItem(className);
            listItem->setData(Qt::UserRole, QVariant::fromValue<QGraphicsItem*>(item));
            ui->listWidget_PolygonList->addItem(listItem);
        }
    }
}

void MainWindow::loadGeometry()
{
    QVector<int> v {10, 10, 800, 600};
    config::base().getValue("windows.main_window.geometry", v);
    setGeometry(v[0], v[1], v[2], v[3]);

    QList<int> splitterSizes;
    if (config::base().getValue("windows.main_window.splitter_sizes", splitterSizes))
        ui->splitter->setSizes(splitterSizes);

    splitterSizes.clear();
    if (config::base().getValue("windows.main_window.splitter2_sizes", splitterSizes))
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
}

void MainWindow::saveAnnotationToFile(const QString& imagePath)
{
    if (!_scenesMap.contains(imagePath))
        return;

    QGraphicsScene* scene = _scenesMap[imagePath];
    QJsonObject json = serializeSceneToJson(scene);

    // Создаем путь к JSON-файлу (тот же путь, но с расширением .json)
    QFileInfo fileInfo(imagePath);
    QString jsonPath = fileInfo.path() + "/" + fileInfo.completeBaseName() + ".json";
    if (json["shapes"].toArray().isEmpty())
    {
        // Удаляем пустой JSON-файл, если нет разметки
        QFile::remove(jsonPath);
        return;
    }

    QFile file(jsonPath);
    if (!file.open(QIODevice::WriteOnly))
    {
        log_error_m << "Failed to open file for writing:" << jsonPath;
        return;
    }

    file.write(QJsonDocument(json).toJson());
    file.close();

    log_info_m << "Annotation saved to:" << jsonPath;
}

void MainWindow::loadAnnotationFromFile(const QString& imagePath)
{
    // Создаем путь к JSON-файлу
    QFileInfo fileInfo(imagePath);
    QString jsonPath = fileInfo.path() + "/" + fileInfo.completeBaseName() + ".json";

    if (!QFile::exists(jsonPath))
        return;

    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly))
    {
        log_error_m << "Failed to open annotation file:" << jsonPath;
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull())
    {
        log_error_m << "Invalid JSON format in file:" << jsonPath;
        return;
    }

    deserializeJsonToScene(_scene, doc.object());
    log_info_m << "Annotation loaded from:" << jsonPath;
}

void MainWindow::deserializeJsonToScene(QGraphicsScene* scene, const QJsonObject& json)
{
    if (!scene)
        return;

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
            rect->setRect(QRectF(x, y, width, height));
            rect->setData(0, className);
        }
        else if (type == "circle")
        {
            qreal x = shapeObj["x"].toDouble();
            qreal y = shapeObj["y"].toDouble();
            qreal radius = shapeObj["radius"].toDouble();

            qgraph::Circle* circle = new qgraph::Circle(scene, QPointF(x, y));
            circle->setRealRadius(radius);
            circle->setData(0, className);
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
        }
    }
}

qgraph::VideoRect* MainWindow::findVideoRect(QGraphicsScene* scene)
{
    if (!scene) return nullptr;

    foreach(QGraphicsItem* item, scene->items()) {
        if (auto* videoRect = dynamic_cast<qgraph::VideoRect*>(item)) {
            return videoRect;
        }
    }
    return nullptr;
}

void MainWindow::resetViewToDefault()
{
    if (!_scene || !_videoRect)
        return;

    // Вычисляем масштаб для полного отображения
    QRectF viewRect = ui->graphView->viewport()->rect();
    QRectF sceneRect = _videoRect->sceneBoundingRect();

    qreal xScale = viewRect.width() / sceneRect.width();
    qreal yScale = viewRect.height() / sceneRect.height();
    qreal scale = qMin(xScale, yScale);

    // Сбрасываем трансформацию
    ui->graphView->resetTransform();
    ui->graphView->scale(scale, scale);
    ui->graphView->centerOn(sceneRect.center());

    // Обновляем сохраненное состояние
    if (!_currentImagePath.isEmpty())
    {
        _scrollStates[_currentImagePath] = {
            ui->graphView->horizontalScrollBar()->value(),
            ui->graphView->verticalScrollBar()->value(),
            scale,
            sceneRect.center()
        };
    }
}

void MainWindow::toggleRightSplitter()
{
    // Предполагаем, что правый сплиттер - это ui->splitter2
    QSplitter* rightSplitter = ui->splitter2;

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

        title = QString("%1 - QLabelMe").arg(displayPath);
    }
    else
    {
        title = "QLabelMe";
    }

    setWindowTitle(title);
}

void MainWindow::updateFolderPathDisplay()
{
    if (_currentFolderPath.isEmpty())
    {
        _folderPathLabel->clear();
        _folderPathLabel->setToolTip("");
    }
    else
    {
        QString displayPath = QDir::toNativeSeparators(_currentFolderPath);
        _folderPathLabel->setText(displayPath);
        _folderPathLabel->setToolTip(displayPath); // Полный путь в подсказке

        // Можно добавить иконку папки
        QIcon folderIcon = style()->standardIcon(QStyle::SP_DirIcon);
        _folderPathLabel->setPixmap(folderIcon.pixmap(16, 16));
    }
}

QJsonObject MainWindow::serializeSceneToJson(QGraphicsScene* scene)
{
    QJsonObject root;
    QJsonArray shapesArray;

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
            QRectF r = rect->rect();
            shapeObj["x"] = r.x();
            shapeObj["y"] = r.y();
            shapeObj["width"] = r.width();
            shapeObj["height"] = r.height();
        }
        else if (auto* circle = dynamic_cast<qgraph::Circle*>(item))
        {
            shapeObj["type"] = "circle";
            QPointF center = circle->realCenter();
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

        shapesArray.append(shapeObj);
    }

    root["shapes"] = shapesArray;
    return root;
}

void MainWindow::saveCurrentViewState() {
    if (_currentImagePath.isEmpty()) return;

    _scrollStates[_currentImagePath] = {
        ui->graphView->horizontalScrollBar()->value(),
        ui->graphView->verticalScrollBar()->value(),
        //getCurrentZoom(),
        ui->graphView->transform().m11(),
        ui->graphView->mapToScene(ui->graphView->viewport()->rect().center())
    };
}

void MainWindow::restoreViewState(const QString& filePath) {
    if (_scrollStates.contains(filePath)) {
        const ScrollState& state = _scrollStates[filePath];

        // Сначала устанавливаем масштаб
        ui->graphView->resetTransform();
        ui->graphView->scale(state.zoom, state.zoom);

        // Затем прокрутку
        ui->graphView->horizontalScrollBar()->setValue(state.hScroll);
        ui->graphView->verticalScrollBar()->setValue(state.vScroll);

        // И центрируем (для точного восстановления)
        ui->graphView->centerOn(state.center);
    } else {
        // Первый просмотр - подгоняем изображение
        fitImageToView();
    }
}

void MainWindow::fitImageToView()
{
    if (!_scene || !_videoRect) return;

    QRectF viewRect = ui->graphView->viewport()->rect();
    QRectF sceneRect = _videoRect->sceneBoundingRect();

    qreal xScale = viewRect.width() / sceneRect.width();
    qreal yScale = viewRect.height() / sceneRect.height();
    qreal scale = qMin(xScale, yScale);

    ui->graphView->resetTransform();
    ui->graphView->scale(scale, scale);
    ui->graphView->centerOn(sceneRect.center());

    // Сохраняем новое состояние
    saveCurrentViewState();
}

void MainWindow::fileList_ItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    if (!current) return;

    // Сохраняем состояние предыдущего изображения
    if (previous && !_currentImagePath.isEmpty()) {
        saveCurrentViewState();
    }

    // if (previous && !_currentImagePath.isEmpty())
    // {
    //     _scrollStates[_currentImagePath] = {
    //         ui->graphView->horizontalScrollBar()->value(),
    //         ui->graphView->verticalScrollBar()->value(),
    //         ui->graphView->getCurrentZoom()
    //     };
    // }

    // Получаем путь к новому изображению
    QString filePath = current->data(Qt::UserRole).toString();
    _currentImagePath = filePath;

    // Проверяем, есть ли уже сцена для этого изображения
    if (!_scenesMap.contains(filePath))
    {
        // Создаем новую сцену
        QGraphicsScene* newScene = new QGraphicsScene(this);
        QPixmap pix(filePath);
        if (pix.isNull())
        {
            log_error_m << "Failed to load image:" << filePath;
            return;
        }

        // Масштабируем изображение и добавляем его на сцену
        pix = pix.scaled(ui->graphView->size(), Qt::KeepAspectRatioByExpanding);
        qgraph::VideoRect* videoRect = new qgraph::VideoRect(newScene);
        videoRect->setPixmap(pix);
        _scenesMap[filePath] = newScene;

        // Загружаем аннотации, если они есть
        loadAnnotationFromFile(filePath);
    }

    // Устанавливаем текущую сцену
    ui->graphView->setScene(_scenesMap[filePath]);
    _scene = _scenesMap[filePath];
    _videoRect = findVideoRect(_scene);

    // Восстанавливаем состояние для этого изображения
    restoreViewState(filePath);

    // Определяем, нужно ли подгонять изображение
    // bool shouldFit = _scrollStates.contains(filePath);
    // fitImageToView(shouldFit);


    // Подгоняем изображение под размер viewport
    //fitImageToView();

    // Восстанавливаем состояние прокрутки
    // if (_scrollStates.contains(filePath))
    // {
    //     const ScrollState& state = _scrollStates[filePath];
    //     ui->graphView->horizontalScrollBar()->setValue(state.hScroll);
    //     ui->graphView->verticalScrollBar()->setValue(state.vScroll);
    //     ui->graphView->setZoom(state.zoom); // если есть масштаб
    // }
    // else
    // {
    //     // Если первый раз — начальное положение
    //     ui->graphView->horizontalScrollBar()->setValue(0);
    //     ui->graphView->verticalScrollBar()->setValue(0);
    // //  ui->graphView->setZoom(1.0);
    // }

    // Обновляем список разметок для новой сцены
    updatePolygonListForCurrentScene();

    // Помечаем текущий элемент как просмотренный
    current->setCheckState(Qt::Checked);
}

void MainWindow::on_actRect_triggered()
{
    _btnRectFlag = true;

    _drawingRectangle = true;
    _drawingCircle = false;
    _drawingLine = false;
}


void MainWindow::on_actCircle_triggered()
{
    _btnCircleFlag = true;

    _drawingCircle = true;
    _drawingLine = false;
    _drawingRectangle= false;

}


void MainWindow::on_actLine_triggered()
{
    _btnLineFlag = true;

    _drawingLine = true;
    _drawingRectangle= false;
    _drawingCircle = false;
}

void MainWindow::wheelEvent(QWheelEvent* event) {
    // Сохраняем состояние при изменении масштаба
    saveCurrentViewState();
    QMainWindow::wheelEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    // При изменении размера сохраняем текущее состояние
    saveCurrentViewState();
    QMainWindow::resizeEvent(event);
    restoreViewState(_currentImagePath);
}

