#pragma once

#include <QMessageBox>
#include <functional>

typedef std::function<void(QMessageBox*)> BoxExtFunc;

QMessageBox::StandardButton messageBox(QWidget* parent,
                                       QMessageBox::Icon icon,
                                       QString message,
                                       int closeTimeout = 0,
                                       BoxExtFunc boxExt = {});
