#include "defaultapps.h"

#include <QComboBox>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStringList>

#include "./ui_defaultapps.h"
#include "config-utils.h"
#include "simple-ini.h"

namespace {
constexpr auto kMimeSection = "Default Applications";
constexpr auto kManagedNotificationBegin = "# >>> labwc-tweaks notifications begin >>>";
constexpr auto kManagedNotificationEnd = "# <<< labwc-tweaks notifications end <<<";

QString sectionName(const char *name)
{
    return QString::fromLatin1(name);
}
}

DefaultApps::DefaultApps(QWidget *parent)
    : QWidget(parent), ui(new Ui::pageDefaultApps)
{
    ui->setupUi(this);
}

DefaultApps::~DefaultApps()
{
    delete ui;
}

QList<DefaultApps::Category> DefaultApps::categories() const
{
    return {
        { QStringLiteral("browser"), QStringLiteral("labwc-default-web-browser"), QString(),
          { QStringLiteral("x-scheme-handler/http"), QStringLiteral("x-scheme-handler/https"),
            QStringLiteral("text/html") },
          ui->webBrowser },
        { QStringLiteral("image"), QStringLiteral("labwc-default-image-viewer"), QString(),
          { QStringLiteral("image/jpeg"), QStringLiteral("image/png"),
            QStringLiteral("image/webp"), QStringLiteral("image/gif"),
            QStringLiteral("image/svg+xml") },
          ui->imageViewer },
        { QStringLiteral("filemanager"), QStringLiteral("labwc-default-file-manager"),
          QStringLiteral("FileManager"),
          { QStringLiteral("inode/directory"), QStringLiteral("x-scheme-handler/file"),
            QStringLiteral("x-scheme-handler/trash") },
          ui->fileManager },
        { QStringLiteral("terminal"), QStringLiteral("labwc-default-terminal"),
          QStringLiteral("TerminalEmulator"),
          { QStringLiteral("x-scheme-handler/terminal"),
            QStringLiteral("application/x-terminal-emulator") },
          ui->terminal },
        { QStringLiteral("video"), QStringLiteral("labwc-default-video-player"), QString(),
          { QStringLiteral("video/mp4"), QStringLiteral("video/x-matroska"),
            QStringLiteral("video/webm"), QStringLiteral("video/x-msvideo"),
            QStringLiteral("video/quicktime") },
          ui->videoPlayer },
        { QStringLiteral("music"), QStringLiteral("labwc-default-music-player"), QString(),
          { QStringLiteral("audio/mpeg"), QStringLiteral("audio/flac"),
            QStringLiteral("audio/ogg"), QStringLiteral("audio/x-wav"),
            QStringLiteral("audio/mp4") },
          ui->musicPlayer },
        { QStringLiteral("archive"), QStringLiteral("labwc-default-archive-manager"), QString(),
          { QStringLiteral("application/zip"), QStringLiteral("application/x-tar"),
            QStringLiteral("application/x-7z-compressed"),
            QStringLiteral("application/x-rar-compressed"),
            QStringLiteral("application/x-xz-compressed-tar") },
          ui->archiveManager },
        { QStringLiteral("email"), QStringLiteral("labwc-default-email-client"), QString(),
          { QStringLiteral("x-scheme-handler/mailto"), QStringLiteral("message/rfc822") },
          ui->emailClient },
        { QStringLiteral("calendar"), QStringLiteral("labwc-default-calendar"), QString(),
          { QStringLiteral("text/calendar"), QStringLiteral("x-scheme-handler/webcal"),
            QStringLiteral("x-scheme-handler/webcals") },
          ui->calendar },
        { QStringLiteral("pdf"), QStringLiteral("labwc-default-pdf-viewer"), QString(),
          { QStringLiteral("application/pdf") },
          ui->pdfViewer },
        { QStringLiteral("editor"), QStringLiteral("labwc-default-text-editor"), QString(),
          { QStringLiteral("text/plain"), QStringLiteral("text/markdown"),
            QStringLiteral("application/json"), QStringLiteral("application/xml"),
            QStringLiteral("text/x-log") },
          ui->textEditor },
    };
}

void DefaultApps::populateDesktopCombo(QComboBox *combo, const QString &currentId)
{
    combo->clear();
    combo->addItem(tr("System default"), QString());
    for (const DesktopEntry &entry : m_entries) {
        combo->addItem(entry.name, entry.id);
    }
    ensureCurrentIdPresent(combo, currentId);

    int index = combo->findData(currentId);
    if (index < 0) {
        index = 0;
    }
    combo->setCurrentIndex(index);
}

QString DefaultApps::desktopIdForProgram(const QString &program) const
{
    const QString simplified = program.trimmed();
    if (simplified.isEmpty()) {
        return QString();
    }

    const QString basename = QFileInfo(simplified).fileName();
    for (const DesktopEntry &entry : m_entries) {
        if (desktopExecProgram(entry.execLine) == basename || entry.id.startsWith(basename)) {
            return entry.id;
        }
    }
    return QString();
}

DesktopEntry DefaultApps::entryForId(const QString &id) const
{
    return m_entriesById.value(id);
}

void DefaultApps::ensureCurrentIdPresent(QComboBox *combo, const QString &desktopId)
{
    if (desktopId.isEmpty() || combo->findData(desktopId) >= 0) {
        return;
    }
    combo->addItem(desktopId + tr(" (not installed)"), desktopId);
}

QString DefaultApps::currentMimeDefault(const SimpleIniFile &mimeApps,
                                        const QStringList &mimeTypes) const
{
    for (const QString &mimeType : mimeTypes) {
        QString value = mimeApps.value(sectionName(kMimeSection), mimeType);
        if (value.isEmpty()) {
            continue;
        }
        return value.section(QLatin1Char(';'), 0, 0).trimmed();
    }
    return QString();
}

QString DefaultApps::currentDesktopId(const Category &category, const SimpleIniFile &mimeApps,
                                      const SimpleIniFile &helpers) const
{
    if (!category.helperKey.isEmpty()) {
        const QString helperValue = helpers.value(QString(), category.helperKey).trimmed();
        const QString helperDesktop = desktopIdForProgram(helperValue);
        if (!helperDesktop.isEmpty()) {
            return helperDesktop;
        }
    }
    return currentMimeDefault(mimeApps, category.mimeTypes);
}

bool DefaultApps::writeWrapper(const QString &wrapperName, const QString &command) const
{
    const QString wrapperPath = userLocalBinPath(wrapperName);
    if (command.trimmed().isEmpty()) {
        QFile::remove(wrapperPath);
        return true;
    }

    const QString script = shellScriptHeader()
            + QStringLiteral("exec ") + command.trimmed() + QStringLiteral(" \"$@\"\n");
    return writeTextFile(wrapperPath, script, QFileDevice::ReadOwner | QFileDevice::WriteOwner
                                                  | QFileDevice::ExeOwner
                                                  | QFileDevice::ReadGroup
                                                  | QFileDevice::ExeGroup
                                                  | QFileDevice::ReadOther
                                                  | QFileDevice::ExeOther);
}

bool DefaultApps::writeNotificationBlock(const QString &command) const
{
    QString block;
    const QString cleaned = command.trimmed();
    if (cleaned.isEmpty()) {
        block = QStringLiteral("# notification daemon disabled");
    } else {
        const QStringList parts = QProcess::splitCommand(cleaned);
        if (parts.isEmpty()) {
            block = QStringLiteral("# notification daemon disabled");
        } else {
            QStringList quotedParts;
            for (const QString &part : parts) {
                quotedParts.push_back(shellSingleQuote(part));
            }
            const QString quotedCommand = quotedParts.join(QLatin1Char(' '));
            const QString processName = QFileInfo(parts.first()).fileName();
            block = QStringLiteral("if command -v %1 >/dev/null 2>&1; then\n")
                            .arg(shellSingleQuote(parts.first()))
                    + QStringLiteral("  if ! pgrep -u \"$(id -u)\" -x %1 >/dev/null 2>&1; then\n")
                              .arg(shellSingleQuote(processName))
                    + QStringLiteral("    %1 >/dev/null 2>&1 &\n").arg(quotedCommand)
                    + QStringLiteral("  fi\nfi");
        }
    }

    return replaceManagedBlock(userConfigPath(QStringLiteral("labwc/autostart")),
                               sectionName(kManagedNotificationBegin),
                               sectionName(kManagedNotificationEnd), block, shellScriptHeader());
}

void DefaultApps::activate()
{
    m_entries = scanDesktopEntries();
    m_entriesById.clear();
    for (const DesktopEntry &entry : m_entries) {
        m_entriesById[entry.id] = entry;
    }

    SimpleIniFile mimeApps;
    mimeApps.load(userConfigPath(QStringLiteral("mimeapps.list")));
    SimpleIniFile helpers;
    helpers.load(userConfigPath(QStringLiteral("xfce4/helpers.rc")));
    SimpleIniFile sessionTools;
    sessionTools.load(appConfigPath(QStringLiteral("default-apps.ini")));

    for (const Category &category : categories()) {
        const QString currentId = currentDesktopId(category, mimeApps, helpers);
        populateDesktopCombo(category.combo, currentId);
    }

    ui->screenshotTool->setEditable(true);
    ui->notificationDaemon->setEditable(true);

    QStringList screenshotCommands;
    if (commandExists(QStringLiteral("grim")) && commandExists(QStringLiteral("slurp"))
        && commandExists(QStringLiteral("swappy"))) {
        screenshotCommands.push_back(QStringLiteral("grim -g \"$(slurp)\" - | swappy -f -"));
    }
    if (commandExists(QStringLiteral("grim"))) {
        screenshotCommands.push_back(
                QStringLiteral("grim \"$HOME/Pictures/screenshot-$(date +%s).png\""));
    }
    if (commandExists(QStringLiteral("spectacle"))) {
        screenshotCommands.push_back(QStringLiteral("spectacle -r"));
    }
    if (commandExists(QStringLiteral("flameshot"))) {
        screenshotCommands.push_back(QStringLiteral("flameshot gui"));
    }
    screenshotCommands.removeDuplicates();
    ui->screenshotTool->clear();
    ui->screenshotTool->addItems(screenshotCommands);

    QStringList notificationCommands;
    for (const QString &candidate :
         { QStringLiteral("mako"), QStringLiteral("swaync"), QStringLiteral("dunst") }) {
        if (commandExists(candidate)) {
            notificationCommands.push_back(candidate);
        }
    }
    ui->notificationDaemon->clear();
    ui->notificationDaemon->addItems(notificationCommands);

    const QString currentScreenshot =
            sessionTools.value(QStringLiteral("tools"), QStringLiteral("screenshot_command"),
                               screenshotCommands.isEmpty() ? QString() : screenshotCommands.first());
    if (ui->screenshotTool->findText(currentScreenshot) < 0 && !currentScreenshot.isEmpty()) {
        ui->screenshotTool->addItem(currentScreenshot);
    }
    ui->screenshotTool->setCurrentText(currentScreenshot);

    const QString currentNotification =
            sessionTools.value(QStringLiteral("tools"), QStringLiteral("notification_command"),
                               notificationCommands.isEmpty() ? QString()
                                                              : notificationCommands.first());
    if (ui->notificationDaemon->findText(currentNotification) < 0
        && !currentNotification.isEmpty()) {
        ui->notificationDaemon->addItem(currentNotification);
    }
    ui->notificationDaemon->setCurrentText(currentNotification);
}

bool DefaultApps::saveMimeDefaults(const SimpleIniFile &mimeApps) const
{
    return mimeApps.save(userConfigPath(QStringLiteral("mimeapps.list")));
}

void DefaultApps::onApply()
{
    SimpleIniFile mimeApps;
    mimeApps.load(userConfigPath(QStringLiteral("mimeapps.list")));
    SimpleIniFile helpers;
    helpers.load(userConfigPath(QStringLiteral("xfce4/helpers.rc")));
    SimpleIniFile sessionTools;

    for (const Category &category : categories()) {
        const QString desktopId = category.combo->currentData().toString();
        for (const QString &mimeType : category.mimeTypes) {
            if (desktopId.isEmpty()) {
                mimeApps.remove(sectionName(kMimeSection), mimeType);
            } else {
                mimeApps.setValue(sectionName(kMimeSection), mimeType, desktopId);
            }
        }

        const DesktopEntry entry = entryForId(desktopId);
        const QString cleanedExec = cleanedDesktopExec(entry.execLine);
        writeWrapper(category.wrapperName, cleanedExec);

        if (!category.helperKey.isEmpty()) {
            const QString helperValue = desktopExecProgram(cleanedExec);
            if (helperValue.isEmpty()) {
                helpers.remove(QString(), category.helperKey);
            } else {
                helpers.setValue(QString(), category.helperKey, helperValue);
            }
        }
    }

    saveMimeDefaults(mimeApps);
    helpers.save(userConfigPath(QStringLiteral("xfce4/helpers.rc")));

    const QString terminalId = ui->terminal->currentData().toString();
    writeWrapper(QStringLiteral("xdg-terminal-exec"),
                 cleanedDesktopExec(entryForId(terminalId).execLine));

    const QString screenshotCommand = ui->screenshotTool->currentText().trimmed();
    const QString notificationCommand = ui->notificationDaemon->currentText().trimmed();
    sessionTools.setValue(QStringLiteral("tools"), QStringLiteral("screenshot_command"),
                          screenshotCommand);
    sessionTools.setValue(QStringLiteral("tools"), QStringLiteral("notification_command"),
                          notificationCommand);
    sessionTools.save(appConfigPath(QStringLiteral("default-apps.ini")));

    writeWrapper(QStringLiteral("labwc-default-screenshot"), screenshotCommand);
    writeNotificationBlock(notificationCommand);
}
