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

//#include "qgraphics/circle.h"
//#include "qgraphics/square.h"
//#include "qgraphics/functions.h"
//#include "qutils/message_box.h"

#include <QApplication>
#include <QHostInfo>
#include <QScreen>
#include <QLabel>
#include <QFileInfo>
#include <QInputDialog>
#include <QGraphicsItem>
#include <QPixmap>
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

    //ui->graphView->setScene(&_scene);
    //_videoRect = new qgraph::VideoRect(&_scene);

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
//    if (_socket)
//        _socket->disconnect();
}

//void MainWindow::message(const pproto::Message::Ptr& message)
//{
//    if (message->processed())
//        return;

//    if (lst::FindResult fr = _funcInvoker.findCommand(message->command()))
//    {
//        if (command::pool().commandIsSinglproc(message->command()))
//            message->markAsProcessed();
//        _funcInvoker.call(message, fr);
//    }
//}

//void MainWindow::socketConnected(pproto::SocketDescriptor)
//{
//    connectActiveMessage();
//    enableButtons(true);
//    disableAdminMode();
//}

//void MainWindow::socketDisconnected(pproto::SocketDescriptor)
//{
//    enableButtons(false);
//    disableAdminMode();

//    if (_isConnected)
//        _labelConnectStatus->setText(u8"Переподключение");
//    else
//        _labelConnectStatus->setText(u8"Нет подключения");

//    QMetaObject::invokeMethod(this, "reconnectCheck", Qt::QueuedConnection);
//}

//void MainWindow::on_actExit_triggered(bool)
//{
//    close();
//}

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

//    config::base().setValue("windows.main_window.tab_index", ui->tabWidget->currentIndex());

//    QByteArray ba = ui->tableJournal->horizontalHeader()->saveState().toBase64();
//    config::base().setValue("windows.main_window.event_journal_header", QString::fromLatin1(ba));
}

//QString MainWindow::labelStyleSheet(const QString& bgrColor)
//{
//    QString s = "border: 1px solid; border-radius: 8px; color: white; "
//                "border-color: rgb(85, 85, 0); background-color: %1; "
//                "margin: 3px;";
//    return s.arg(bgrColor);
//}

//void MainWindow::enableButtons(bool val)
//{
//    ui->actReceiveVideo->setEnabled(val);
//    ui->actAdminMode->setEnabled(val);
//    ui->actRoiGeometry->setEnabled(val);
//    ui->actRoiGeometrySave->setEnabled(val);
//}

//void MainWindow::connectActiveMessage()
//{
//    QString msg = u8"Подключено к %1:%2";
//    _labelConnectStatus->setText(msg.arg(_socket->peerPoint().address().toString())
//                                    .arg(_socket->peerPoint().port()));
//}

void MainWindow::on_actionClose_triggered()
{
    QApplication::quit();
}


void MainWindow::on_actionOpen_triggered()
{
    // Create File Dialog
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open Image"), "/home");

    // Выводит путь файла
    //ui->image->setText(fileName);
    QPixmap pix(fileName);
    int width = ui->image->width();
    int height = ui->image->height();
    ui->image->setPixmap(pix.scaled(width, height, Qt::KeepAspectRatio));
}
