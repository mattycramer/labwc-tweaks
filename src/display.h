#ifndef DISPLAY_H
#define DISPLAY_H

#include <QMap>
#include <QVector>
#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class pageDisplay;
}
QT_END_NAMESPACE

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;

class DisplayPage : public QWidget
{
    Q_OBJECT

public:
    explicit DisplayPage(QWidget *parent = nullptr);
    ~DisplayPage();

    void activate();
    void onApply();

private:
    struct OutputMode
    {
        QString id;
        QString label;
        int width = 0;
        int height = 0;
        double refresh = 0.0;
        bool preferred = false;
        bool current = false;
    };

    struct OutputInfo
    {
        QString name;
        QString description;
        bool enabled = false;
        int positionX = 0;
        int positionY = 0;
        double scale = 1.0;
        QVector<OutputMode> modes;
    };

    struct OutputWidgets
    {
        QCheckBox *enabled = nullptr;
        QLabel *description = nullptr;
        QComboBox *mode = nullptr;
        QDoubleSpinBox *scale = nullptr;
    };

    bool loadOutputs();
    void rebuildOutputWidgets();
    void clearOutputWidgets();
    QString inferredLayoutMode() const;
    QString selectedPrimary() const;
    OutputMode selectedMode(const QString &outputName) const;
    QString kanshiConfig() const;
    void signalKanshiReload() const;

    Ui::pageDisplay *ui;
    QVector<OutputInfo> m_outputs;
    QMap<QString, OutputWidgets> m_outputWidgets;
};

#endif // DISPLAY_H
