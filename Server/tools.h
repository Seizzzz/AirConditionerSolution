#ifndef TOOLS_H
#define TOOLS_H

#include <QJsonObject>
#include <QJsonDocument>

inline QJsonObject string2jsonobj(const QString& str)
{
    //return QJsonDocument::fromJson(str.toLocal8Bit().data()).object();

    return QJsonDocument::fromJson(str.toUtf8()).object();
}

inline QString jsonobj2string(const QJsonObject& obj)
{
    //return QString(QJsonDocument(obj).toJson());

    return QString(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

#endif // TOOLS_H
