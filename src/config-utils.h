#ifndef CONFIG_UTILS_H
#define CONFIG_UTILS_H

#include <QFileDevice>
#include <QString>

QString userConfigPath(const QString &relativePath);
QString userDataPath(const QString &relativePath);
QString userLocalBinPath(const QString &fileName);
QString appConfigPath(const QString &fileName);
QString activeLabwcThemePath(const QString &themeName);

bool commandExists(const QString &program);
bool ensureDirectoryForFile(const QString &path);
bool writeTextFile(const QString &path, const QString &contents,
                   QFileDevice::Permissions permissions = QFileDevice::ReadOwner
                           | QFileDevice::WriteOwner | QFileDevice::ReadGroup
                           | QFileDevice::ReadOther);
QString readTextFile(const QString &path);
bool replaceManagedBlock(const QString &path, const QString &beginMarker,
                         const QString &endMarker, const QString &block,
                         const QString &header = QString());
QString shellSingleQuote(const QString &value);
QString shellScriptHeader(void);

#endif // CONFIG_UTILS_H
