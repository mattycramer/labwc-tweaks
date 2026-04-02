#include "simple-ini.h"

#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QSaveFile>
#include <QTextStream>

bool SimpleIniFile::load(const QString &path)
{
    m_values.clear();

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream stream(&file);
    QString currentSection;
    while (!stream.atEnd()) {
        QString line = stream.readLine().trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#'))
            || line.startsWith(QLatin1Char(';'))) {
            continue;
        }
        if (line.startsWith(QLatin1Char('[')) && line.endsWith(QLatin1Char(']'))) {
            currentSection = line.mid(1, line.size() - 2).trimmed();
            continue;
        }

        int delimiter = line.indexOf(QLatin1Char('='));
        if (delimiter < 0) {
            continue;
        }

        QString key = line.left(delimiter).trimmed();
        QString value = line.mid(delimiter + 1).trimmed();
        m_values[currentSection][key] = value;
    }

    return true;
}

bool SimpleIniFile::save(const QString &path) const
{
    const QFileInfo info(path);
    if (!info.dir().exists() && !QDir().mkpath(info.dir().path())) {
        return false;
    }

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream stream(&file);

    auto writeSection = [&stream](const QString &section, const QMap<QString, QString> &values,
                                  bool leadingBlank) {
        if (values.isEmpty()) {
            return;
        }
        if (leadingBlank) {
            stream << '\n';
        }
        if (!section.isEmpty()) {
            stream << '[' << section << "]\n";
        }
        for (auto it = values.cbegin(); it != values.cend(); ++it) {
            stream << it.key() << '=' << it.value() << '\n';
        }
    };

    bool wroteRoot = false;
    if (m_values.contains(QString())) {
        writeSection(QString(), m_values.value(QString()), false);
        wroteRoot = !m_values.value(QString()).isEmpty();
    }

    bool wroteAnyNamed = false;
    for (auto it = m_values.cbegin(); it != m_values.cend(); ++it) {
        if (it.key().isEmpty()) {
            continue;
        }
        writeSection(it.key(), it.value(), wroteRoot || wroteAnyNamed);
        wroteAnyNamed = wroteAnyNamed || !it.value().isEmpty();
    }

    if (!file.commit()) {
        return false;
    }
    return true;
}

QString SimpleIniFile::value(const QString &section, const QString &key, const QString &defaultValue) const
{
    if (!m_values.contains(section)) {
        return defaultValue;
    }
    return m_values.value(section).value(key, defaultValue);
}

void SimpleIniFile::setValue(const QString &section, const QString &key, const QString &value)
{
    m_values[section][key] = value;
}

void SimpleIniFile::remove(const QString &section, const QString &key)
{
    if (!m_values.contains(section)) {
        return;
    }
    m_values[section].remove(key);
    if (m_values[section].isEmpty()) {
        m_values.remove(section);
    }
}

bool SimpleIniFile::contains(const QString &section, const QString &key) const
{
    return m_values.contains(section) && m_values.value(section).contains(key);
}
