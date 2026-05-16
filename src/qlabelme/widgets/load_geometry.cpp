#include "load_geometry.h"

#include "shared/list.h"
#include <QGuiApplication>
#include <QScreen>

std::vector<int> windowScreenCenter23(int screenIndex)
{
    QList<QScreen*> screens = QGuiApplication::screens();
    if (lst::inRange(screenIndex, 0, screens.count()))
    {
        QRect sg = screens[screenIndex]->geometry();
        QRect wr {0, 0, int(sg.width() * (2./3)), int(sg.height() * (2./3))};
        std::vector<int> wg (size_t(4));
        wg[0] = sg.width()  / 2 - wr.width()  / 2;
        wg[1] = sg.height() / 2 - wr.height() / 2;
        wg[2] = wr.width();
        wg[3] = wr.height();
        return wg;
    }
    return {};
}

void windowLoadGeometry(YamlConfig& conf, const std::string& name, QMainWindow* window,
                        std::vector<int>* defaultGeometry)
{
    std::vector<int> wg; // window geometry vector
    conf.getValue(name, wg);

    if (wg.size() != 4)
    {
        wg = {10 /*X*/, 10 /*Y*/, 800 /*Width*/, 600 /*Height*/};
        if (defaultGeometry && (defaultGeometry->size() >= 4))
            wg = *defaultGeometry;
    }
    window->setGeometry(wg[0], wg[1], wg[2], wg[3]);
}


void dialogLoadGeometry(YamlConfig& conf, const std::string& name, QDialog* dialog)
{
    std::vector<int> wg; // window geometry vector
    conf.getValue(name, wg);

    if (wg.size() != 4)
    {
        wg = {10 /*X*/, 10 /*Y*/, 800 /*Width*/, 600 /*Height*/};
        QList<QScreen*> screens = QGuiApplication::screens();
        if (screens.count())
        {
            QRect sg = screens[0]->geometry();
            QRect dr {0, 0, int(dialog->width()), int(dialog->height())};
            wg[0] = sg.width()  / 2 - dr.width()  / 2;
            wg[1] = sg.height() / 2 - dr.height() / 2;
            wg[2] = dr.width();
            wg[3] = dr.height();
        }
    }
    dialog->setGeometry(wg[0], wg[1], wg[2], wg[3]);
}
