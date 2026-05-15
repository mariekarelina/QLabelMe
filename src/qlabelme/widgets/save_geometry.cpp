#include "save_geometry.h"

#include <QGuiApplication>
#include <QScreen>

void windowSaveGeometry(YamlConfig& conf, const std::string& pathName,
                        QMainWindow* window, std::vector<int>* defaultGeometry)
{
    std::vector<int> wg; // window geometry vector
    conf.getValue(pathName, wg);

    if (wg.size() != 4)
    {
        // wg = {10 /*X*/, 10 /*Y*/, 800 /*Width*/, 600 /*Height*/};
        // if (defaultGeometry)
        // {
        //     wg = *defaultGeometry;
        // }
        // QList<QScreen*> screens = QGuiApplication::screens();
        // if (!screens.isEmpty())
        // {
        //     QRect sg = screens[0]->geometry();
        //     //QRect windowGeometry = this->geometry();
        //     QRect wg2 {0, 0, int(sg.width()  * (2./3)), int(sg.height() * (2./3))};
        //     wg[0] = screenGeometry.width()  / 2 - windowGeometry.width()  / 2;
        //     wg[1] = screenGeometry.height() / 2 - windowGeometry.height() / 2;
        //     wg[2] = windowGeometry.width();
        //     wg[3] = windowGeometry.height();
        // }
    }
    window->setGeometry(wg[0], wg[1], wg[2], wg[3]);

}
