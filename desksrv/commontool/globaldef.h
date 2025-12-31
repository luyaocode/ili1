#ifndef GLOBALDEF_H
#define GLOBALDEF_H
#include <QDebug>
#ifdef DEBUG
    #define LOG_MESSAGE(module, msg) qDebug() << QString("[%1]:%2").arg(module).arg(msg);
#else
    #define LOG_MESSAGE(module, msg) ((void)0);
#endif

#endif  // GLOBALDEF_H
