#ifndef SERVER_H
#define SERVER_H

#include <QObject>

#include <QWebSocket>
#include <QWebSocketServer>

#include <QSqlDatabase>
#include <QSqlQuery>

#include <QJsonObject>
#include <QJsonDocument>

#define JSONAME_TYPE "type"
#define JSONAME_ROOMID "roomID"
#define JSONAME_USERID "userID"
#define JSONAME_WNDSPD "wind_speed"
#define JSONAME_TEMP "tepture"
#define JSONAME_MODE "model"

#define DB_ADDR "127.0.0.1"
#define DB_PORT 3306
#define DB_DATABASE_NAME "hotel"
#define DB_TABLE_NAME "opt"

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
    void onNewConnection();
    void onDisconnect();
    void process(const QString&);

    QWebSocketServer* sock;
    QList<QWebSocket*> lstClt;

private:
    void connectMySQL();

    QSqlDatabase db;
};


#endif // SERVER_H
