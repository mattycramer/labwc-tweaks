#include "appearance.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QProcess>
#include <QStringList>

#include "config-utils.h"
#include "find-themes.h"
#include "macros.h"
#include "simple-ini.h"
#include "settings.h"
#include "pair.h"
#include "./ui_appearance.h"

namespace {
QString tomlQuoted(const QString &value)
{
    QString escaped = value;
    escaped.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    escaped.replace(QLatin1Char('"'), QStringLiteral("\\\""));
    return QStringLiteral("\"%1\"").arg(escaped);
}
}

Appearance::Appearance(QWidget *parent) : QWidget(parent), ui(new Ui::pageAppearance)
{
    ui->setupUi(this);

    ui->wallpaperEngine->clear();
    ui->wallpaperEngine->addItem(tr("Disabled"), QStringLiteral("none"));
    ui->wallpaperEngine->addItem(QStringLiteral("swaybg"), QStringLiteral("swaybg"));
    ui->wallpaperEngine->addItem(QStringLiteral("swww"), QStringLiteral("swww"));
    ui->wallpaperEngine->addItem(QStringLiteral("wpaperd"), QStringLiteral("wpaperd"));

    ui->wallpaperMode->clear();
    ui->wallpaperMode->addItem(tr("Fill"), QStringLiteral("fill"));
    ui->wallpaperMode->addItem(tr("Fit"), QStringLiteral("fit"));
    ui->wallpaperMode->addItem(tr("Stretch"), QStringLiteral("stretch"));

    ui->panelTarget->clear();
    ui->panelTarget->addItem(tr("Disabled"), QStringLiteral("none"));
    ui->panelTarget->addItem(QStringLiteral("Waybar"), QStringLiteral("waybar"));
    ui->panelTarget->addItem(QStringLiteral("SFWBar"), QStringLiteral("sfwbar"));

    connect(ui->openboxTheme, &QComboBox::currentTextChanged, this, [&]() { updateThemercPath(); });
    connect(ui->browseWallpaper, &QPushButton::clicked, this, [&]() {
        QString selected = QFileDialog::getOpenFileName(this, tr("Select wallpaper"), QDir::homePath());
        if (!selected.isEmpty()) {
            ui->wallpaperPath->setText(selected);
        }
    });
}

Appearance::~Appearance()
{
    delete ui;
}

void Appearance::activate()
{
    SimpleIniFile appearanceState;
    appearanceState.load(appConfigPath(QStringLiteral("appearance.ini")));

    /* Labwc Theme */
    settingsAddXmlStr("/labwc_config/theme/name", "");
    QStringList labwcThemes = findLabwcThemes();
    ui->openboxTheme->clear();
    ui->openboxTheme->addItems(labwcThemes);
    ui->openboxTheme->setCurrentIndex(labwcThemes.indexOf(getStr("/labwc_config/theme/name")));
    updateThemercPath();

    ui->gtkTheme->clear();
    ui->gtkTheme->addItems(findGtkThemes());
    const QString gtkTheme =
            appearanceState.value(QStringLiteral("theme"), QStringLiteral("gtk_theme"),
                                  QStringLiteral("Adwaita"));
    int gtkIndex = ui->gtkTheme->findText(gtkTheme);
    ui->gtkTheme->setCurrentIndex(gtkIndex >= 0 ? gtkIndex : 0);

    /* Corner Radius */
    settingsAddXmlInt("/labwc_config/theme/cornerRadius", 8);
    ui->cornerRadius->setValue(getInt("/labwc_config/theme/cornerRadius"));
    ui->cornerRadius->setToolTip(tr("Radius of server side decoration top corners"));

    /* Drop Shadows */
    settingsAddXmlBoo("/labwc_config/theme/dropShadows", false);
    ui->dropShadows->setChecked(getBool("/labwc_config/theme/dropShadows"));
    ui->dropShadows->setToolTip(tr("Render drop-shadows behind windows"));

    /* Drop Shadows On Tiled */
    settingsAddXmlBoo("/labwc_config/theme/dropShadowsOnTiled", false);
    ui->dropShadowsOnTiled->setChecked(getBool("/labwc_config/theme/dropShadowsOnTiled"));
    ui->dropShadowsOnTiled->setToolTip(tr("Render drop-shadows behind tiled windows"));

    // Disable it when Drop Shadows is unchecked
    ui->dropShadowsOnTiled->setEnabled(ui->dropShadows->isChecked());
    connect(ui->dropShadows, &QCheckBox::toggled, ui->dropShadowsOnTiled, &QWidget::setEnabled,
            Qt::UniqueConnection);

    /* Icon Theme */
    settingsAddXmlStr("/labwc_config/theme/icon", "");
    QStringList themes = findIconThemes(LAB_ICON_THEME_TYPE_ICON);
    ui->iconTheme->clear();
    ui->iconTheme->addItems(themes);
    ui->iconTheme->setCurrentIndex(themes.indexOf(getStr("/labwc_config/theme/icon")));

    const QString wallpaperEngine =
            appearanceState.value(QStringLiteral("wallpaper"), QStringLiteral("engine"),
                                  commandExists(QStringLiteral("swaybg")) ? QStringLiteral("swaybg")
                                                                          : QStringLiteral("none"));
    const int engineIndex = ui->wallpaperEngine->findData(wallpaperEngine);
    ui->wallpaperEngine->setCurrentIndex(engineIndex >= 0 ? engineIndex : 0);
    ui->wallpaperPath->setText(
            appearanceState.value(QStringLiteral("wallpaper"), QStringLiteral("path")));
    const QString wallpaperMode =
            appearanceState.value(QStringLiteral("wallpaper"), QStringLiteral("mode"),
                                  QStringLiteral("fill"));
    const int wallpaperModeIndex = ui->wallpaperMode->findData(wallpaperMode);
    ui->wallpaperMode->setCurrentIndex(wallpaperModeIndex >= 0 ? wallpaperModeIndex : 0);

    QString panelStyle =
            appearanceState.value(QStringLiteral("panel"), QStringLiteral("style_target"));
    if (panelStyle.isEmpty()) {
        if (QFileInfo::exists(userConfigPath(QStringLiteral("waybar/style.css")))) {
            panelStyle = QStringLiteral("waybar");
        } else if (QFileInfo::exists(userConfigPath(QStringLiteral("sfwbar/sfwbar.config")))) {
            panelStyle = QStringLiteral("sfwbar");
        } else {
            panelStyle = QStringLiteral("none");
        }
    }
    const int panelIndex = ui->panelTarget->findData(panelStyle);
    ui->panelTarget->setCurrentIndex(panelIndex >= 0 ? panelIndex : 0);
    ui->panelBackground->setText(
            appearanceState.value(QStringLiteral("panel"), QStringLiteral("background"),
                                  QStringLiteral("rgba(18, 24, 32, 0.92)")));
    ui->panelSurface->setText(
            appearanceState.value(QStringLiteral("panel"), QStringLiteral("surface"),
                                  QStringLiteral("rgba(33, 46, 64, 0.84)")));
    ui->panelText->setText(
            appearanceState.value(QStringLiteral("panel"), QStringLiteral("text"),
                                  QStringLiteral("#e5edf5")));
    ui->panelAccent->setText(
            appearanceState.value(QStringLiteral("panel"), QStringLiteral("accent"),
                                  QStringLiteral("#7dd3fc")));

    /* Decoration */
    settingsAddXmlStr("/labwc_config/core/decoration", "server");
    ui->decoration->setToolTip(tr("Specify decorations for xdg-shell windows"));

    QVector<QSharedPointer<Pair>> decorations;
    ui->decoration->clear();
    decorations.append(
            QSharedPointer<Pair>(new Pair("server", tr("Server Side Decoration (SSD)"))));
    decorations.append(
            QSharedPointer<Pair>(new Pair("client", tr("Client Side Decoration (CSD)"))));

    QString current_decoration = getStr("/labwc_config/core/decoration");
    int decoration_index = -1;
    foreach (auto decoration, decorations) {
        ui->decoration->addItem(decoration.get()->description(),
                                QVariant(decoration.get()->value()));
        ++decoration_index;
        if (current_decoration == decoration.get()->value()) {
            ui->decoration->setCurrentIndex(decoration_index);
        }
    }

    /* Maximized Decoration */
    settingsAddXmlStr("/labwc_config/theme/maximizedDecoration", "titlebar");
    ui->maximizedDecoration->setToolTip(tr("Show server side decorations on maximized windows"));

    QVector<QSharedPointer<Pair>> maximized_decorations;
    ui->maximizedDecoration->clear();
    maximized_decorations.append(QSharedPointer<Pair>(new Pair("titlebar", tr("Titlebar"))));
    maximized_decorations.append(QSharedPointer<Pair>(new Pair("none", tr("None"))));

    QString current_maximized_decoration = getStr("/labwc_config/theme/maximizedDecoration");
    int maximized_decoration_index = -1;
    foreach (auto maximized_decoration, maximized_decorations) {
        ui->maximizedDecoration->addItem(maximized_decoration.get()->description(),
                                         QVariant(maximized_decoration.get()->value()));
        ++maximized_decoration_index;
        if (current_maximized_decoration == maximized_decoration.get()->value()) {
            ui->maximizedDecoration->setCurrentIndex(maximized_decoration_index);
        }
    }
}

void Appearance::onApply()
{
    setInt("/labwc_config/theme/cornerRadius", ui->cornerRadius->value());
    setStr("/labwc_config/theme/name", TEXT(ui->openboxTheme));
    setBool("/labwc_config/theme/dropShadows", ui->dropShadows->isChecked());
    setBool("/labwc_config/theme/dropShadowsOnTiled", ui->dropShadowsOnTiled->isChecked());
    setStr("/labwc_config/theme/icon", TEXT(ui->iconTheme));
    setStr("/labwc_config/core/decoration", DATA(ui->decoration));
    setStr("/labwc_config/theme/maximizedDecoration", DATA(ui->maximizedDecoration));

    SimpleIniFile appearanceState;
    appearanceState.setValue(QStringLiteral("theme"), QStringLiteral("gtk_theme"),
                             TEXT(ui->gtkTheme));
    appearanceState.setValue(QStringLiteral("wallpaper"), QStringLiteral("engine"),
                             ui->wallpaperEngine->currentData().toString());
    appearanceState.setValue(QStringLiteral("wallpaper"), QStringLiteral("path"),
                             ui->wallpaperPath->text().trimmed());
    appearanceState.setValue(QStringLiteral("wallpaper"), QStringLiteral("mode"),
                             ui->wallpaperMode->currentData().toString());
    appearanceState.setValue(QStringLiteral("panel"), QStringLiteral("style_target"),
                             ui->panelTarget->currentData().toString());
    appearanceState.setValue(QStringLiteral("panel"), QStringLiteral("background"),
                             ui->panelBackground->text().trimmed());
    appearanceState.setValue(QStringLiteral("panel"), QStringLiteral("surface"),
                             ui->panelSurface->text().trimmed());
    appearanceState.setValue(QStringLiteral("panel"), QStringLiteral("text"),
                             ui->panelText->text().trimmed());
    appearanceState.setValue(QStringLiteral("panel"), QStringLiteral("accent"),
                             ui->panelAccent->text().trimmed());
    appearanceState.save(appConfigPath(QStringLiteral("appearance.ini")));

    writeGtkSettings();
    writeWallpaperConfig();
    writePanelStyle();
}

void Appearance::updateThemercPath()
{
    ui->themercPath->setText(activeLabwcThemePath(ui->openboxTheme->currentText()));
}

QString Appearance::wallpaperBlock() const
{
    const QString engine = ui->wallpaperEngine->currentData().toString();
    const QString path = ui->wallpaperPath->text().trimmed();
    const QString mode = ui->wallpaperMode->currentData().toString();

    if (engine == QStringLiteral("none") || path.isEmpty()) {
        return QStringLiteral("# wallpaper disabled");
    }

    if (engine == QStringLiteral("swaybg")) {
        return QStringLiteral("if command -v swaybg >/dev/null 2>&1; then\n"
                              "  pkill -x swaybg >/dev/null 2>&1 || true\n"
                              "  swaybg -i %1 -m %2 >/dev/null 2>&1 &\n"
                              "fi")
                .arg(shellSingleQuote(path), shellSingleQuote(mode));
    }

    if (engine == QStringLiteral("swww")) {
        QString resizeMode = QStringLiteral("crop");
        if (mode == QStringLiteral("fit")) {
            resizeMode = QStringLiteral("fit");
        } else if (mode == QStringLiteral("stretch")) {
            resizeMode = QStringLiteral("stretch");
        }
        return QStringLiteral("if command -v swww >/dev/null 2>&1 && command -v swww-daemon >/dev/null 2>&1; then\n"
                              "  pgrep -x swww-daemon >/dev/null 2>&1 || swww-daemon >/dev/null 2>&1 &\n"
                              "  sleep 1\n"
                              "  swww img %1 --transition-type none --resize %2 >/dev/null 2>&1 || true\n"
                              "fi")
                .arg(shellSingleQuote(path), shellSingleQuote(resizeMode));
    }

    if (engine == QStringLiteral("wpaperd")) {
        return QStringLiteral("if command -v wpaperd >/dev/null 2>&1; then\n"
                              "  pkill -x wpaperd >/dev/null 2>&1 || true\n"
                              "  wpaperd >/dev/null 2>&1 &\n"
                              "fi");
    }

    return QStringLiteral("# wallpaper disabled");
}

void Appearance::writeGtkSettings() const
{
    const QString gtkTheme = TEXT(ui->gtkTheme);
    const QString iconTheme = TEXT(ui->iconTheme);

    for (const QString &path :
         { userConfigPath(QStringLiteral("gtk-3.0/settings.ini")),
           userConfigPath(QStringLiteral("gtk-4.0/settings.ini")) }) {
        SimpleIniFile settings;
        settings.load(path);
        settings.setValue(QStringLiteral("Settings"), QStringLiteral("gtk-theme-name"), gtkTheme);
        settings.setValue(QStringLiteral("Settings"), QStringLiteral("gtk-icon-theme-name"),
                          iconTheme);
        settings.save(path);
    }
}

void Appearance::writeWallpaperConfig() const
{
    const QString engine = ui->wallpaperEngine->currentData().toString();
    if (engine == QStringLiteral("wpaperd") && !ui->wallpaperPath->text().trimmed().isEmpty()) {
        const QString contents = QStringLiteral("[default]\npath = %1\nmode = %2\n")
                                         .arg(tomlQuoted(ui->wallpaperPath->text().trimmed()),
                                              tomlQuoted(
                                                      ui->wallpaperMode->currentData().toString()));
        writeTextFile(userConfigPath(QStringLiteral("wpaperd/config.toml")), contents);
    }

    replaceManagedBlock(userConfigPath(QStringLiteral("labwc/autostart")),
                        QStringLiteral("# >>> labwc-tweaks wallpaper begin >>>"),
                        QStringLiteral("# <<< labwc-tweaks wallpaper end <<<"), wallpaperBlock(),
                        shellScriptHeader());
}

void Appearance::writePanelStyle() const
{
    const QString target = ui->panelTarget->currentData().toString();
    const QString background = ui->panelBackground->text().trimmed();
    const QString surface = ui->panelSurface->text().trimmed();
    const QString text = ui->panelText->text().trimmed();
    const QString accent = ui->panelAccent->text().trimmed();
    const QString disabled = QStringLiteral("/* panel overrides disabled */");

    if (target == QStringLiteral("waybar")) {
        const QString block = QStringLiteral("@define-color panel %1;\n"
                                             "@define-color panel_alt %2;\n"
                                             "@define-color panel_hover %2;\n"
                                             "@define-color border %4;\n"
                                             "@define-color border_strong %4;\n"
                                             "@define-color text %3;\n"
                                             "@define-color muted %3;\n"
                                             "@define-color amber %4;\n"
                                             "@define-color sky %4;\n"
                                             "@define-color cyan %4;\n"
                                             "@define-color mint %4;\n"
                                             "@define-color rose %4;\n"
                                             "@define-color red %4;\n"
                                             "@define-color mauve %4;")
                                      .arg(background, surface, text, accent);
        replaceManagedBlock(userConfigPath(QStringLiteral("waybar/style.css")),
                            QStringLiteral("/* >>> labwc-tweaks panel begin >>> */"),
                            QStringLiteral("/* <<< labwc-tweaks panel end <<< */"), block);
        replaceManagedBlock(userConfigPath(QStringLiteral("sfwbar/sfwbar.config")),
                            QStringLiteral("/* >>> labwc-tweaks panel begin >>> */"),
                            QStringLiteral("/* <<< labwc-tweaks panel end <<< */"), disabled);
    } else if (target == QStringLiteral("sfwbar")) {
        const QString block = QStringLiteral("window#sfwbar {\n"
                                             "  background-color: %1;\n"
                                             "  color: %3;\n"
                                             "  border-color: %4;\n"
                                             "}\n"
                                             "window#sfwbar button,\n"
                                             "window#sfwbar label,\n"
                                             "window#sfwbar image {\n"
                                             "  background-color: %2;\n"
                                             "  color: %3;\n"
                                             "}\n"
                                             "window#sfwbar button:hover,\n"
                                             "window#sfwbar button:checked,\n"
                                             "window#sfwbar button.active {\n"
                                             "  background-color: %4;\n"
                                             "  color: %1;\n"
                                             "}")
                                      .arg(background, surface, text, accent);
        replaceManagedBlock(userConfigPath(QStringLiteral("sfwbar/sfwbar.config")),
                            QStringLiteral("/* >>> labwc-tweaks panel begin >>> */"),
                            QStringLiteral("/* <<< labwc-tweaks panel end <<< */"), block);
        replaceManagedBlock(userConfigPath(QStringLiteral("waybar/style.css")),
                            QStringLiteral("/* >>> labwc-tweaks panel begin >>> */"),
                            QStringLiteral("/* <<< labwc-tweaks panel end <<< */"), disabled);
    } else {
        replaceManagedBlock(userConfigPath(QStringLiteral("waybar/style.css")),
                            QStringLiteral("/* >>> labwc-tweaks panel begin >>> */"),
                            QStringLiteral("/* <<< labwc-tweaks panel end <<< */"), disabled);
        replaceManagedBlock(userConfigPath(QStringLiteral("sfwbar/sfwbar.config")),
                            QStringLiteral("/* >>> labwc-tweaks panel begin >>> */"),
                            QStringLiteral("/* <<< labwc-tweaks panel end <<< */"), disabled);
    }
}
