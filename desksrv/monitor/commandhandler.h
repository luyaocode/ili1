#ifndef COMMANDHANDLER_H
#define COMMANDHANDLER_H

#include <QStringList>
#include <QDate>

class CommandHandler
{
public:
    CommandHandler();

    bool processArguments(const QStringList &args);
    bool shouldExitAfterProcessing() const;
    bool isForegroundMode() const;
    bool needGui(const QStringList &args) const;

private:
    void         showHelp();
    void         showVersion();
    void         showUsage();
    void         queryUsage(bool showGui = false);
    QList<QDate> getCurrentMonthUsageDates();
    QList<QDate> getCurrentYearUsageDates();
    QList<QDate> getTotalUsageDates();

private:
    bool m_shouldExit;
    bool m_foregroundMode;
};

#endif  // COMMANDHANDLER_H
