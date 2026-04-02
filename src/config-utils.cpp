#include "config-utils.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStandardPaths>
#include <QStringList>

QString userConfigPath(const QString &relativePath)
{
    return QDir::homePath() + QStringLiteral("/.config/") + relativePath;
}

QString userDataPath(const QString &relativePath)
{
    return QDir::homePath() + QStringLiteral("/.local/share/") + relativePath;
}

QString userLocalBinPath(const QString &fileName)
{
    return QDir::homePath() + QStringLiteral("/.local/bin/") + fileName;
}

QString appConfigPath(const QString &fileName)
{
    return userConfigPath(QStringLiteral("labwc-tweaks/") + fileName);
}

QString activeLabwcThemePath(const QString &themeName)
{
    QStringList roots;
    roots.push_back(QDir::homePath() + QStringLiteral("/.themes"));
    const QStringList dataRoots =
            QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
    for (const QString &root : dataRoots) {
        roots.push_back(root + QStringLiteral("/themes"));
    }

    for (const QString &root : roots) {
        const QStringList candidates = {
            QStringLiteral("%1/%2/labwc/themerc").arg(root, themeName),
            QStringLiteral("%1/%2/openbox-3/themerc").arg(root, themeName),
        };
        for (const QString &candidate : candidates) {
            if (QFileInfo::exists(candidate)) {
                return candidate;
            }
        }
    }

    return QString();
}

bool commandExists(const QString &program)
{
    return !QStandardPaths::findExecutable(program).isEmpty();
}

bool ensureDirectoryForFile(const QString &path)
{
    const QFileInfo info(path);
    QDir dir = info.dir();
    if (dir.exists()) {
        return true;
    }
    return dir.mkpath(QStringLiteral("."));
}

bool writeTextFile(const QString &path, const QString &contents, QFileDevice::Permissions permissions)
{
    if (!ensureDirectoryForFile(path)) {
        return false;
    }

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QByteArray data = contents.toUtf8();
    if (!data.endsWith('\n')) {
        data.push_back('\n');
    }

    if (file.write(data) != data.length()) {
        return false;
    }
    if (!file.commit()) {
        return false;
    }

    QFile target(path);
    target.setPermissions(permissions);
    return true;
}

QString readTextFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }
    return QString::fromUtf8(file.readAll());
}

bool replaceManagedBlock(const QString &path, const QString &beginMarker, const QString &endMarker,
                         const QString &block, const QString &header)
{
    QString contents = readTextFile(path);
    if (contents.isEmpty() && !QFileInfo::exists(path)) {
        contents = header;
    }
    if (!contents.isEmpty() && !contents.endsWith('\n')) {
        contents.push_back('\n');
    }

    QString managedBlock = beginMarker + QLatin1Char('\n') + block.trimmed() + QLatin1Char('\n')
            + endMarker + QLatin1Char('\n');

    int start = contents.indexOf(beginMarker);
    int finish = contents.indexOf(endMarker);
    if (start >= 0 && finish >= start) {
        finish += endMarker.size();
        if (finish < contents.size() && contents.at(finish) == QLatin1Char('\n')) {
            ++finish;
        }
        contents.replace(start, finish - start, managedBlock);
    } else {
        if (!contents.isEmpty() && !contents.endsWith("\n\n")) {
            contents.push_back('\n');
        }
        contents += managedBlock;
    }

    return writeTextFile(path, contents);
}

QString shellSingleQuote(const QString &value)
{
    QString escaped = value;
    escaped.replace(QStringLiteral("'"), QStringLiteral("'\"'\"'"));
    return QStringLiteral("'") + escaped + QStringLiteral("'");
}

QString shellScriptHeader(void)
{
    return QStringLiteral("#!/bin/sh\nset -eu\n");
}
