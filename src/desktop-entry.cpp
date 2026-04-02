#include "desktop-entry.h"

#include <QDir>
#include <QDirIterator>
#include <QMap>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>

#include "simple-ini.h"

static QStringList applicationDirs()
{
    QStringList dirs;
    dirs.push_back(QDir::homePath() + QStringLiteral("/.local/share/applications"));

    const QStringList dataRoots =
            QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
    for (const QString &root : dataRoots) {
        dirs.push_back(root + QStringLiteral("/applications"));
    }

    dirs.removeDuplicates();
    return dirs;
}

QList<DesktopEntry> scanDesktopEntries(void)
{
    QMap<QString, QString> filesById;
    for (const QString &dirPath : applicationDirs()) {
        QDir dir(dirPath);
        if (!dir.exists()) {
            continue;
        }

        QDirIterator iterator(dirPath, QStringList() << QStringLiteral("*.desktop"),
                              QDir::Files, QDirIterator::NoIteratorFlags);
        while (iterator.hasNext()) {
            const QString filePath = iterator.next();
            const QString id = QFileInfo(filePath).fileName();
            if (!filesById.contains(id)) {
                filesById[id] = filePath;
            }
        }
    }

    QList<DesktopEntry> entries;
    for (auto it = filesById.cbegin(); it != filesById.cend(); ++it) {
        SimpleIniFile file;
        if (!file.load(it.value())) {
            continue;
        }

        const QString name = file.value(QStringLiteral("Desktop Entry"), QStringLiteral("Name"));
        const QString exec = file.value(QStringLiteral("Desktop Entry"), QStringLiteral("Exec"));
        const QString hidden =
                file.value(QStringLiteral("Desktop Entry"), QStringLiteral("Hidden")).toLower();
        const QString noDisplay =
                file.value(QStringLiteral("Desktop Entry"), QStringLiteral("NoDisplay")).toLower();

        if (name.isEmpty() || exec.isEmpty() || hidden == QStringLiteral("true")
            || noDisplay == QStringLiteral("true")) {
            continue;
        }

        DesktopEntry entry;
        entry.id = it.key();
        entry.name = name;
        entry.execLine = exec;
        entry.terminal =
                file.value(QStringLiteral("Desktop Entry"), QStringLiteral("Terminal")).toLower()
                == QStringLiteral("true");
        entries.push_back(entry);
    }

    std::sort(entries.begin(), entries.end(), [](const DesktopEntry &lhs, const DesktopEntry &rhs) {
        return lhs.name.toLower() < rhs.name.toLower();
    });

    return entries;
}

QString cleanedDesktopExec(const QString &execLine)
{
    QString cleaned = execLine;
    cleaned.replace(QRegularExpression(QStringLiteral("%[fFuUdDnNickvm]")), QString());
    cleaned.replace(QStringLiteral("%%"), QStringLiteral("%"));
    cleaned = cleaned.simplified();
    return cleaned;
}

QString desktopExecProgram(const QString &execLine)
{
    const QString cleaned = cleanedDesktopExec(execLine);
    const QStringList parts = QProcess::splitCommand(cleaned);
    if (parts.isEmpty()) {
        return QString();
    }
    return parts.first();
}
