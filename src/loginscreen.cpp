#include "loginscreen.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QProcess>

#include "./ui_loginscreen.h"
#include "config-utils.h"
#include "find-themes.h"
#include "simple-ini.h"

LoginScreen::LoginScreen(QWidget *parent) : QWidget(parent), ui(new Ui::pageLoginScreen)
{
    ui->setupUi(this);

    connect(ui->browseBackground, &QPushButton::clicked, this, [&]() {
        QString selected =
                QFileDialog::getOpenFileName(this, tr("Select login background"), QDir::homePath());
        if (!selected.isEmpty()) {
            ui->backgroundPath->setText(selected);
        }
    });
}

LoginScreen::~LoginScreen()
{
    delete ui;
}

void LoginScreen::activate()
{
    ui->gtkTheme->clear();
    ui->gtkTheme->addItems(findGtkThemes());

    SimpleIniFile state;
    state.load(appConfigPath(QStringLiteral("login-screen.ini")));
    SimpleIniFile regreetToml;
    regreetToml.load(QStringLiteral("/etc/greetd/regreet.toml"));

    const QString gtkTheme =
            state.value(QStringLiteral("regreet"), QStringLiteral("gtk_theme"),
                        regreetToml.value(QStringLiteral("GTK"), QStringLiteral("theme_name"),
                                          QStringLiteral("\"Adwaita\""))
                                .remove(QLatin1Char('"')));
    int themeIndex = ui->gtkTheme->findText(gtkTheme);
    ui->gtkTheme->setCurrentIndex(themeIndex >= 0 ? themeIndex : 0);

    ui->backgroundPath->setText(
            state.value(QStringLiteral("regreet"), QStringLiteral("background_path")));
    ui->panelBackground->setText(
            state.value(QStringLiteral("regreet"), QStringLiteral("panel_background"),
                        QStringLiteral("rgba(12, 8, 20, 0.86)")));
    ui->textColor->setText(
            state.value(QStringLiteral("regreet"), QStringLiteral("text_color"),
                        QStringLiteral("#f5f3ff")));
    ui->accentColor->setText(
            state.value(QStringLiteral("regreet"), QStringLiteral("accent_color"),
                        QStringLiteral("#c4b5fd")));
    ui->fieldBackground->setText(
            state.value(QStringLiteral("regreet"), QStringLiteral("field_background"),
                        QStringLiteral("rgba(23, 15, 35, 0.96)")));
}

void LoginScreen::onApply()
{
    const QString backgroundPath = ui->backgroundPath->text().trimmed();
    if (!backgroundPath.isEmpty() && !QFileInfo::exists(backgroundPath)) {
        QMessageBox::warning(this, tr("Login Screen"),
                             tr("The selected background image does not exist."));
        return;
    }

    SimpleIniFile state;
    state.setValue(QStringLiteral("regreet"), QStringLiteral("gtk_theme"), ui->gtkTheme->currentText());
    state.setValue(QStringLiteral("regreet"), QStringLiteral("background_path"), backgroundPath);
    state.setValue(QStringLiteral("regreet"), QStringLiteral("panel_background"),
                   ui->panelBackground->text().trimmed());
    state.setValue(QStringLiteral("regreet"), QStringLiteral("text_color"),
                   ui->textColor->text().trimmed());
    state.setValue(QStringLiteral("regreet"), QStringLiteral("accent_color"),
                   ui->accentColor->text().trimmed());
    state.setValue(QStringLiteral("regreet"), QStringLiteral("field_background"),
                   ui->fieldBackground->text().trimmed());
    state.save(appConfigPath(QStringLiteral("login-screen.ini")));

    if (!commandExists(QStringLiteral("pkexec"))) {
        QMessageBox::warning(this, tr("Login Screen"),
                             tr("pkexec is not available, so system login-screen settings cannot be applied."));
        return;
    }

    const QString helperPath =
            QStringLiteral(PROJECT_LIBEXEC_DIR) + QStringLiteral("/labwc-tweaks-apply-login-screen");

    QProcess process;
    process.start(QStringLiteral("pkexec"),
                  QStringList() << helperPath << QStringLiteral("--background") << backgroundPath
                                << QStringLiteral("--gtk-theme") << ui->gtkTheme->currentText()
                                << QStringLiteral("--panel-bg")
                                << ui->panelBackground->text().trimmed()
                                << QStringLiteral("--text-color")
                                << ui->textColor->text().trimmed()
                                << QStringLiteral("--accent-color")
                                << ui->accentColor->text().trimmed()
                                << QStringLiteral("--field-bg")
                                << ui->fieldBackground->text().trimmed());
    process.waitForFinished(-1);

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        QMessageBox::warning(this, tr("Login Screen"),
                             tr("Failed to update greetd/regreet.\n\n%1")
                                     .arg(QString::fromUtf8(process.readAllStandardError())));
    }
}
