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
    void ProcessType1(QJsonObject json, QWebSocket* clt);
    void ProcessType2(QJsonObject json, QWebSocket* clt);
    void ProcessType3(QJsonObject json, QWebSocket* clt);
    void ProcessType4(QJsonObject json);
    void ProcessType5(QJsonObject json);
    void ProcessType6(QJsonObject json);
    void ProcessType7(QJsonObject json);
    void ProcessType8(QJsonObject json);
    int ComputeMoney(QJsonObject json);
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
