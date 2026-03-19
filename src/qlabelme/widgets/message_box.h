#pragma once

#include <QMessageBox>

QMessageBox::StandardButton messageBox(QWidget* parent,
                                       QMessageBox::Icon icon,
                                       QString message,
                                       int closeTimeout = 0);
