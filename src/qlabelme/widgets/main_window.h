#pragma once

#include "shared/simple_ptr.h"
#include "pproto/func_invoker.h"
#include "pproto/transport/tcp.h"

//#include "commands/commands.h"
//#include "commands/error.h"

//#include "qgraphics/video_rect.h"
//#include "qutils/video_widget.h"

#include <QMainWindow>
#include <QFileDialog>
#include <QCloseEvent>
#include <QGraphicsScene>

using namespace std;
using namespace pproto;
using namespace pproto::transport;

namespace Ui {
class MainWindow;
}

class QLabel;

class MainWindow : public QMainWindow
{
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    bool init();
    void deinit();

private slots:
    void on_actOpen_triggered(bool);
    void on_actClose_triggered(bool);

private:
    Q_OBJECT
    void closeEvent(QCloseEvent*) override;

    void loadGeometry();
    void saveGeometry();

private:
    Ui::MainWindow *ui;
    static QUuidEx _applId;

    //QGraphicsScene _scene;
    //qgraph::VideoRect* _videoRect = {nullptr};

    QString _windowTitle;
    QLabel* _labelConnectStatus;

    // Признак UltraHD монитора
    bool _ultraHD = {false};
};
