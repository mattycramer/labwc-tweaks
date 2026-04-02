#ifndef DEFAULTAPPS_H
#define DEFAULTAPPS_H

#include <QMap>
#include <QWidget>

#include "desktop-entry.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class pageDefaultApps;
}
QT_END_NAMESPACE

class QComboBox;

class DefaultApps : public QWidget
{
    Q_OBJECT

public:
    explicit DefaultApps(QWidget *parent = nullptr);
    ~DefaultApps();

    void activate();
    void onApply();

private:
    struct Category
    {
        QString key;
        QString wrapperName;
        QString helperKey;
        QStringList mimeTypes;
        QComboBox *combo = nullptr;
    };

    QList<Category> categories() const;
    void populateDesktopCombo(QComboBox *combo, const QString &currentId);
    QString desktopIdForProgram(const QString &program) const;
    DesktopEntry entryForId(const QString &id) const;
    void ensureCurrentIdPresent(QComboBox *combo, const QString &desktopId);
    QString currentDesktopId(const Category &category, const class SimpleIniFile &mimeApps,
                             const class SimpleIniFile &helpers) const;
    QString currentMimeDefault(const class SimpleIniFile &mimeApps,
                               const QStringList &mimeTypes) const;
    bool writeWrapper(const QString &wrapperName, const QString &command) const;
    bool writeNotificationBlock(const QString &command) const;
    bool saveMimeDefaults(const class SimpleIniFile &mimeApps) const;

    Ui::pageDefaultApps *ui;
    QList<DesktopEntry> m_entries;
    QMap<QString, DesktopEntry> m_entriesById;
};

#endif // DEFAULTAPPS_H
