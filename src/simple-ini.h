#ifndef SIMPLE_INI_H
#define SIMPLE_INI_H

#include <QMap>
#include <QString>

class SimpleIniFile
{
public:
    bool load(const QString &path);
    bool save(const QString &path) const;

    QString value(const QString &section, const QString &key,
                  const QString &defaultValue = QString()) const;
    void setValue(const QString &section, const QString &key, const QString &value);
    void remove(const QString &section, const QString &key);
    bool contains(const QString &section, const QString &key) const;

private:
    QMap<QString, QMap<QString, QString>> m_values;
};

#endif // SIMPLE_INI_H
