#ifndef TOOL_H
#define TOOL_H

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <functional>

#include "quickitem.h"
#include "commontool/x11struct.h"

class KeyWithModifier;

void createConfigFile(const QString &filePath);

bool getKeyAndModifer(const QuickItem &item, KeyWithModifier &keymod);

#endif // TOOL_H
