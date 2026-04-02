#ifndef DESKTOP_ENTRY_H
#define DESKTOP_ENTRY_H

#include <QList>
#include <QString>

struct DesktopEntry
{
    QString id;
    QString name;
    QString execLine;
    bool terminal = false;
};

QList<DesktopEntry> scanDesktopEntries(void);
QString cleanedDesktopExec(const QString &execLine);
QString desktopExecProgram(const QString &execLine);

#endif // DESKTOP_ENTRY_H
