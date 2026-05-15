#pragma once

#include "shared/config/yaml_config.h"

#include <QtCore>
#include <QMainWindow>

void windowSaveGeometry(YamlConfig&, const std::string& pathName, QMainWindow* window,
                        std::vector<int>* defaultGeometry = nullptr);
