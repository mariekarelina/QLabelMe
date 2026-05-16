#pragma once

#include "shared/config/yaml_config.h"

#include <QtCore>
#include <QDialog>
#include <QMainWindow>

std::vector<int> windowScreenCenter23(int screenIndex = 0);

void windowLoadGeometry(YamlConfig&, const std::string& name, QMainWindow* window,
                        std::vector<int>* defaultGeometry = nullptr);

void dialogLoadGeometry(YamlConfig&, const std::string& name, QDialog* dialog);

