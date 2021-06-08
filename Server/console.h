#ifndef SERVER_H
#define SERVER_H

#include <QObject>

#include <QWebSocket>
#include <QWebSocketServer>
#include <QMap>

#include <QSqlDatabase>
#include <QSqlQuery>

#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>

#include "../tools.h"

#define DEBUG
#ifdef DEBUG
    #include <QDebug>
    #include <QSqlError>

    #define DEBUG_CONNECTED
    #define DEBUG_DISCONNECTED
    #define DEBUG_RCV_CONTENT
    #define DEBUG_SEND_CONTENT
    #define DEBUG_DB_QUERY
    #define DEBUG_DB_ERR
#endif

struct LinkInfo
{
    QString roomID;
    QString userID;
    double roomTemp; //房间温度

    int targetTemp; //目标温度
    int targetWndSpd; //风速

    LinkInfo() {
        roomID = "";
        userID = "";
        roomTemp = -1;
    }
};

struct ConfigInfo
{
    double FeeRateLight;
    double FeeRateGentle;
    double FeeRateStrong;

    ConfigInfo() {
        FeeRateLight = 0;
        FeeRateGentle = 0;
        FeeRateStrong = 0;
    }
};
static ConfigInfo config;

using InfoIter = QMap<QString, LinkInfo>::iterator;

class Console : public QObject
{
    Q_OBJECT

public:
    explicit Console(quint16 port, QObject* parent = nullptr);
    ~Console();

private:
    QString rcvType0(const QJsonObject& json, InfoIter);
    QString rcvType1(const QJsonObject& json, InfoIter);
    QString rcvType2(const QJsonObject& json, InfoIter);
    QString rcvType3(const QJsonObject& json, InfoIter);
    QString rcvType6(const QJsonObject& json);
    QString rcvType7(const QJsonObject& json);
    QString rcvType10();
    double getPriceCost(const QJsonObject& json);

private:
    void onNewConnection();
    void onDisconnect();
    void process(const QString&);
    QWebSocketServer* sock;
    QList<QWebSocket*> lstClt;
    QMap<QString, LinkInfo> mapLinkInfo;
    QMap<QString, LinkInfo>::iterator getLinkInfo(QString&);
    bool isRoomExisted(const QString&);

private:
    void connectMySQL();
    QSqlDatabase db;
};


#endif // SERVER_H
