#ifndef SERVER_H
#define SERVER_H

#include <QObject>

#include <QWebSocket>
#include <QWebSocketServer>

#include <QSqlDatabase>
#include <QSqlQuery>

#include <QJsonObject>
#include <QJsonDocument>
#include<QDateTime>
#define JSONAME_TYPE "MsgType"
#define JSONAME_ROOMID "RoomId"
#define JSONAME_USERID "UserId"
#define JSONAME_WNDSPD "WindSpeed"
#define JSONAME_TEMP "Temp"
#define JSONAME_MODE "Mode"
#define JSONAME_AIRDATA "AirData"
#define JSONAME_ACK "Ack"
#define JSONAME_MONEY "Money"
#define JSONAME_DETAIL "CostDetail"
#define JSONAME_STIME "TimeStart"
#define JSONAME_ETIME "TimeEnd"
#define JSONAME_COST "Cost"
#define JSONAME_AIRSTATUE "AirStatus"
#define JSONAME_ISON "IsOn"
#define JSONAME_ALL "AllRecord"


#define DB_ADDR "127.0.0.1"
//#define DB_ADDR "10.128.248.29"
#define DB_PORT 3306
#define DB_DATABASE_NAME "hotel"
//#define DB_DATABASE_NAME "hotelinfo"
#define DB_TABLE_NAME "room"

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

class Console : public QObject
{
    Q_OBJECT

public:
    explicit Console(quint16 port, QObject* parent = nullptr);
    ~Console();

private:
    QJsonObject string2jsonobj(const QString& str);
    QString jsonobj2string(const QJsonObject& obj);

private:
    QString ProcessType1(const QJsonObject& json);
    QString ProcessType2(const QJsonObject& json);
    QString ProcessType3(const QJsonObject& json);
    QString ProcessType4(const QJsonObject& json);
    QString ProcessType5(const QJsonObject& json);
    QString ProcessType6(const QJsonObject& json);
    QString ProcessType7(const QJsonObject& json);
    QString ProcessType8(const QJsonObject& json);
    int ComputeMoney(const QJsonObject& json);
private:
    void onNewConnection();
    void onDisconnect();
    void process(const QString&);
    QWebSocketServer* sock;
    QList<QWebSocket*> lstClt;
private:
    void connectMySQL();
    QSqlDatabase db;
    QDateTime time;
};


#endif // SERVER_H
