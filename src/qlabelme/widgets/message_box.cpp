#include "message_box.h"

#include <QTimer>
#include <QPushButton>
#include <QApplication>

class TimedMessageBox : public QMessageBox
{
public:
    TimedMessageBox(QWidget* parent, int closeTimeout = 0);

private:
    void showEvent(QShowEvent*) override final;

    int _closeTimeout = {0};
    QTimer _closeTimer;
    QString _buttonText;
};

TimedMessageBox::TimedMessageBox(QWidget* parent, int closeTimeout)
    : QMessageBox(parent)
    , _closeTimeout(closeTimeout)
{
    QPushButton* btnOk = addButton(QMessageBox::Ok);
    if (closeTimeout > 0)
    {
        if (btnOk)
            _buttonText = btnOk->text();

        _closeTimer.setInterval(1000);
        connect(&_closeTimer, &QTimer::timeout, this, [this]()
        {
            if (--_closeTimeout >= 0)
            {
                if (QAbstractButton* btn = this->button(QMessageBox::Ok))
                {
                    QString newBtnTxt = QString("%1 (%2)").arg(_buttonText)
                                                          .arg(_closeTimeout);
                    btn->setText(newBtnTxt);
                }
            }
            else
            {
                _closeTimer.stop();
                this->accept();
                //QAbstractButton* btn = this->button(QMessageBox::Ok);
                //if (btn)
                //    btn->animateClick();
            }
        });
    }
}

void TimedMessageBox::showEvent(QShowEvent* e)
{
    QMessageBox::showEvent(e);

    if (_closeTimeout > 0)
        _closeTimer.start();
}

QMessageBox::StandardButton messageBox(QWidget* parent,
                                       QMessageBox::Icon icon,
                                       QString message,
                                       int closeTimeout)
{
    message.replace("  ", "<br>").replace(" ", "&nbsp;");
    TimedMessageBox msgBox {parent, closeTimeout};
    //msgBox.setWindowModality(Qt::WindowModal);
    msgBox.setIcon(icon);
    msgBox.setWindowTitle(qApp->applicationName());
    msgBox.setTextFormat(Qt::RichText);
    msgBox.setText(message);

    int res = msgBox.exec();
    return static_cast<QMessageBox::StandardButton>(res);
}
