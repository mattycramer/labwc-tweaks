#include "display.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLayoutItem>
#include <QProcess>

#include "./ui_display.h"
#include "config-utils.h"
#include "simple-ini.h"

namespace {
QString formatRefresh(double refresh)
{
    int rounded = qRound(refresh);
    if (qAbs(refresh - rounded) < 0.05) {
        return QString::number(rounded);
    }
    return QString::number(refresh, 'f', 2);
}

QString modeId(int width, int height, double refresh)
{
    return QStringLiteral("%1x%2@%3Hz").arg(width).arg(height).arg(formatRefresh(refresh));
}

QString outputSection(const QString &name)
{
    return QStringLiteral("output:%1").arg(name);
}

QString defaultDisplayConfig()
{
    return QStringLiteral("# no outputs selected\n");
}

QString kanshiQuote(const QString &value)
{
    QString escaped = value;
    escaped.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    escaped.replace(QLatin1Char('"'), QStringLiteral("\\\""));
    return QStringLiteral("\"%1\"").arg(escaped);
}
}

DisplayPage::DisplayPage(QWidget *parent) : QWidget(parent), ui(new Ui::pageDisplay)
{
    ui->setupUi(this);
    ui->layoutMode->addItem(tr("Single display"), QStringLiteral("single"));
    ui->layoutMode->addItem(tr("Mirror"), QStringLiteral("mirror"));
    ui->layoutMode->addItem(tr("Extend"), QStringLiteral("extend"));
}

DisplayPage::~DisplayPage()
{
    delete ui;
}

bool DisplayPage::loadOutputs()
{
    m_outputs.clear();

    if (!commandExists(QStringLiteral("wlr-randr"))) {
        ui->statusLabel->setText(tr("wlr-randr is not installed. Display settings can still be saved, but connected outputs cannot be detected right now."));
        return false;
    }

    QProcess process;
    process.start(QStringLiteral("wlr-randr"), QStringList() << QStringLiteral("--json"));
    process.waitForFinished(5000);
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        ui->statusLabel->setText(tr("wlr-randr did not return output information for this session."));
        return false;
    }

    QJsonParseError error;
    const QJsonDocument document =
            QJsonDocument::fromJson(process.readAllStandardOutput(), &error);
    if (error.error != QJsonParseError::NoError || !document.isArray()) {
        ui->statusLabel->setText(tr("Failed to parse wlr-randr JSON output."));
        return false;
    }

    for (const QJsonValue &value : document.array()) {
        if (!value.isObject()) {
            continue;
        }

        const QJsonObject object = value.toObject();
        OutputInfo info;
        info.name = object.value(QStringLiteral("name")).toString();
        info.description = object.value(QStringLiteral("description")).toString(info.name);
        info.enabled = object.value(QStringLiteral("enabled")).toBool();
        info.scale = object.value(QStringLiteral("scale")).toDouble(1.0);

        const QJsonObject position = object.value(QStringLiteral("position")).toObject();
        info.positionX = position.value(QStringLiteral("x")).toInt();
        info.positionY = position.value(QStringLiteral("y")).toInt();

        const QJsonArray modes = object.value(QStringLiteral("modes")).toArray();
        for (const QJsonValue &modeValue : modes) {
            if (!modeValue.isObject()) {
                continue;
            }
            const QJsonObject modeObject = modeValue.toObject();
            OutputMode mode;
            mode.width = modeObject.value(QStringLiteral("width")).toInt();
            mode.height = modeObject.value(QStringLiteral("height")).toInt();
            mode.refresh = modeObject.value(QStringLiteral("refresh")).toDouble();
            mode.preferred = modeObject.value(QStringLiteral("preferred")).toBool();
            mode.current = modeObject.value(QStringLiteral("current")).toBool();
            mode.id = modeId(mode.width, mode.height, mode.refresh);
            mode.label =
                    QStringLiteral("%1 x %2 @ %3 Hz").arg(mode.width).arg(mode.height).arg(
                            formatRefresh(mode.refresh));
            if (mode.preferred) {
                mode.label += tr(" (preferred)");
            } else if (mode.current) {
                mode.label += tr(" (current)");
            }
            info.modes.push_back(mode);
        }

        if (!info.name.isEmpty()) {
            m_outputs.push_back(info);
        }
    }

    ui->statusLabel->setText(m_outputs.isEmpty()
                                     ? tr("No wlroots outputs were reported by wlr-randr.")
                                     : tr("Detected %1 output(s) via wlr-randr.").arg(m_outputs.size()));
    return !m_outputs.isEmpty();
}

void DisplayPage::clearOutputWidgets()
{
    while (QLayoutItem *item = ui->outputsLayout->takeAt(0)) {
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }
    m_outputWidgets.clear();
}

void DisplayPage::rebuildOutputWidgets()
{
    clearOutputWidgets();

    SimpleIniFile state;
    state.load(appConfigPath(QStringLiteral("display.ini")));

    ui->primaryOutput->clear();
    for (const OutputInfo &output : m_outputs) {
        ui->primaryOutput->addItem(output.description, output.name);
    }

    QString preferredPrimary = state.value(QStringLiteral("display"), QStringLiteral("primary"));
    if (preferredPrimary.isEmpty()) {
        preferredPrimary = selectedPrimary();
    }

    const int primaryIndex = ui->primaryOutput->findData(preferredPrimary);
    ui->primaryOutput->setCurrentIndex(primaryIndex >= 0 ? primaryIndex : 0);

    QString layoutMode = state.value(QStringLiteral("display"), QStringLiteral("layout"));
    if (layoutMode.isEmpty()) {
        layoutMode = inferredLayoutMode();
    }
    int layoutIndex = ui->layoutMode->findData(layoutMode);
    if (layoutIndex < 0) {
        layoutIndex = 0;
    }
    ui->layoutMode->setCurrentIndex(layoutIndex);

    for (const OutputInfo &output : m_outputs) {
        QGroupBox *group = new QGroupBox(output.description, this);
        QGridLayout *layout = new QGridLayout(group);
        layout->setHorizontalSpacing(16);
        layout->setVerticalSpacing(8);

        OutputWidgets widgets;
        widgets.enabled = new QCheckBox(tr("Enable"), group);
        const QString section = outputSection(output.name);
        const QString storedEnabled = state.value(section, QStringLiteral("enabled"));
        widgets.enabled->setChecked(storedEnabled.isEmpty() ? output.enabled
                                                            : storedEnabled == QStringLiteral("true"));
        layout->addWidget(widgets.enabled, 0, 0, 1, 2);

        QLabel *modeLabel = new QLabel(tr("Mode"), group);
        layout->addWidget(modeLabel, 1, 0);
        widgets.mode = new QComboBox(group);
        QString currentMode = state.value(section, QStringLiteral("mode"));
        for (const OutputMode &mode : output.modes) {
            widgets.mode->addItem(mode.label, mode.id);
            if (currentMode.isEmpty() && mode.current) {
                currentMode = mode.id;
            }
        }
        int modeIndex = widgets.mode->findData(currentMode);
        widgets.mode->setCurrentIndex(modeIndex >= 0 ? modeIndex : 0);
        layout->addWidget(widgets.mode, 1, 1);

        QLabel *scaleLabel = new QLabel(tr("Scale"), group);
        layout->addWidget(scaleLabel, 2, 0);
        widgets.scale = new QDoubleSpinBox(group);
        widgets.scale->setDecimals(2);
        widgets.scale->setMinimum(0.50);
        widgets.scale->setMaximum(4.00);
        widgets.scale->setSingleStep(0.25);
        const QString storedScale = state.value(section, QStringLiteral("scale"));
        widgets.scale->setValue(storedScale.isEmpty() ? output.scale : storedScale.toDouble());
        layout->addWidget(widgets.scale, 2, 1);

        widgets.description = new QLabel(output.name, group);
        widgets.description->setTextInteractionFlags(Qt::TextSelectableByMouse);
        layout->addWidget(widgets.description, 3, 0, 1, 2);

        ui->outputsLayout->addWidget(group);
        m_outputWidgets[output.name] = widgets;
    }
}

QString DisplayPage::inferredLayoutMode() const
{
    int enabledCount = 0;
    bool samePosition = true;
    int firstX = 0;
    int firstY = 0;
    bool firstSet = false;

    for (const OutputInfo &output : m_outputs) {
        if (!output.enabled) {
            continue;
        }
        ++enabledCount;
        if (!firstSet) {
            firstX = output.positionX;
            firstY = output.positionY;
            firstSet = true;
            continue;
        }
        samePosition = samePosition && output.positionX == firstX && output.positionY == firstY;
    }

    if (enabledCount <= 1) {
        return QStringLiteral("single");
    }
    return samePosition ? QStringLiteral("mirror") : QStringLiteral("extend");
}

QString DisplayPage::selectedPrimary() const
{
    for (const OutputInfo &output : m_outputs) {
        if (output.enabled) {
            return output.name;
        }
    }
    return m_outputs.isEmpty() ? QString() : m_outputs.first().name;
}

DisplayPage::OutputMode DisplayPage::selectedMode(const QString &outputName) const
{
    OutputMode fallback;
    for (const OutputInfo &output : m_outputs) {
        if (output.name != outputName) {
            continue;
        }
        const QString selectedId = m_outputWidgets.value(outputName).mode->currentData().toString();
        for (const OutputMode &mode : output.modes) {
            if (mode.id == selectedId) {
                return mode;
            }
            if (fallback.id.isEmpty() && mode.current) {
                fallback = mode;
            }
        }
        if (fallback.id.isEmpty() && !output.modes.isEmpty()) {
            fallback = output.modes.first();
        }
        break;
    }
    return fallback;
}

QString DisplayPage::kanshiConfig() const
{
    QStringList enabledOutputs;
    for (auto it = m_outputWidgets.cbegin(); it != m_outputWidgets.cend(); ++it) {
        if (it.value().enabled->isChecked()) {
            enabledOutputs.push_back(it.key());
        }
    }
    if (enabledOutputs.isEmpty()) {
        return defaultDisplayConfig();
    }

    const QString primary = ui->primaryOutput->currentData().toString();
    enabledOutputs.removeAll(primary);
    enabledOutputs.push_front(primary);
    enabledOutputs.removeDuplicates();

    const QString layoutMode = ui->layoutMode->currentData().toString();
    QStringList config;

    for (const QString &soloOutput : enabledOutputs) {
        config << QStringLiteral("profile solo-%1 {").arg(soloOutput);
        for (const OutputInfo &output : m_outputs) {
            if (output.name == soloOutput) {
                const OutputMode mode = selectedMode(output.name);
                const double scale = m_outputWidgets.value(output.name).scale->value();
                config << QStringLiteral("  output %1 mode %2 scale %3 position 0,0 enable")
                                  .arg(kanshiQuote(output.name), mode.id,
                                       QString::number(scale, 'f', 2));
            } else {
                config << QStringLiteral("  output %1 disable").arg(kanshiQuote(output.name));
            }
        }
        config << QStringLiteral("}");
        config << QString();
    }

    if (enabledOutputs.size() > 1 && layoutMode != QStringLiteral("single")) {
        config << QStringLiteral("profile labwc-tweaks {");
        int currentX = 0;
        for (const QString &outputName : enabledOutputs) {
            const OutputMode mode = selectedMode(outputName);
            const double scale = m_outputWidgets.value(outputName).scale->value();
            config << QStringLiteral("  output %1 mode %2 scale %3 position %4,0 enable")
                              .arg(kanshiQuote(outputName), mode.id,
                                   QString::number(scale, 'f', 2), QString::number(currentX));
            if (layoutMode == QStringLiteral("extend")) {
                currentX += qRound(mode.width / scale);
            }
        }
        for (const OutputInfo &output : m_outputs) {
            if (!enabledOutputs.contains(output.name)) {
                config << QStringLiteral("  output %1 disable").arg(kanshiQuote(output.name));
            }
        }
        config << QStringLiteral("}");
        config << QString();
    }

    return config.join(QLatin1Char('\n'));
}

void DisplayPage::signalKanshiReload() const
{
    if (!commandExists(QStringLiteral("kanshi"))) {
        return;
    }

    QProcess process;
    process.start(QStringLiteral("pkill"),
                  QStringList() << QStringLiteral("-HUP") << QStringLiteral("-x")
                                << QStringLiteral("kanshi"));
    process.waitForFinished(1000);

    if (process.exitCode() != 0) {
        QProcess::startDetached(QStringLiteral("kanshi"));
    }
}

void DisplayPage::activate()
{
    loadOutputs();
    rebuildOutputWidgets();
}

void DisplayPage::onApply()
{
    SimpleIniFile state;
    state.setValue(QStringLiteral("display"), QStringLiteral("layout"),
                   ui->layoutMode->currentData().toString());
    state.setValue(QStringLiteral("display"), QStringLiteral("primary"),
                   ui->primaryOutput->currentData().toString());

    for (auto it = m_outputWidgets.cbegin(); it != m_outputWidgets.cend(); ++it) {
        state.setValue(outputSection(it.key()), QStringLiteral("enabled"),
                       it.value().enabled->isChecked() ? QStringLiteral("true")
                                                       : QStringLiteral("false"));
        state.setValue(outputSection(it.key()), QStringLiteral("mode"),
                       it.value().mode->currentData().toString());
        state.setValue(outputSection(it.key()), QStringLiteral("scale"),
                       QString::number(it.value().scale->value(), 'f', 2));
    }
    state.save(appConfigPath(QStringLiteral("display.ini")));

    writeTextFile(userConfigPath(QStringLiteral("kanshi/config")), kanshiConfig());
    signalKanshiReload();
}
