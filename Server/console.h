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

#define JSONAME_TYPE "MsgType"
#define JSONAME_ROOMID "RoomId"
#define JSONAME_USERID "UserId"
#define JSONAME_WNDSPD "WindSpeed"
#define JSONAME_TEMP "Temp"
#define JSONAME_MODE "Mode"
#define JSONAME_POWER "TurnOnOff"
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

#define DBNAME_DATABASE "hotel"

#define DBNAME_TABLE_ROOM "room"
#define DBNAME_FIELD_RID "Roomid"
#define DBNAME_FIELD_UID "UserId"
#define DBNAME_FIELD_WNDSPD "WindSpeed"
#define DBNAME_FIELD_TEMP "tempture"
#define DBNAME_FIELD_POWER "AirState"
#define DBNAME_FIELD_TIME "Time"
#define DBNAME_FIELD_MODE "Mode"

#define DBNAME_TABLE_ROOMSTATE "roomstate"
#define DBNAME_FIELD_RID2 "RoomId"
#define DBNAME_FIELD_STATE "State"

#define DB_ADDR "127.0.0.1"
#define DB_PORT 3306

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

struct userInfo
{
    QString roomID;
    QString userID;

    userInfo(QString rid, QString uid) : roomID(rid), userID(uid) {};
};

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
    QString ProcessType0(const QJsonObject& json);
    QString ProcessType1(const QJsonObject& json, QString&);
    QString ProcessType2(const QJsonObject& json);
    QString ProcessType3(const QJsonObject& json);
    QString ProcessType4(const QJsonObject& json);
    QString ProcessType5(const QJsonObject& json);
    QString ProcessType6(const QJsonObject& json);
    QString ProcessType7(const QJsonObject& json);
    QString ProcessType8(const QJsonObject& json);
    double getPriceCost(const QJsonObject& json);

private:
    void onNewConnection();
    void onDisconnect();
    void process(const QString&);
    QWebSocketServer* sock;
    QList<QWebSocket*> lstClt;
    QMap<QString, userInfo> sockInfo;

private:
    void connectMySQL();
    QSqlDatabase db;
};


#endif // SERVER_H
