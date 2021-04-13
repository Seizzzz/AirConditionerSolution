#ifndef ROOM_H
#define ROOM_H

#include <QMainWindow>
#include <QTimer>
#include <QString>

#include <QWebSocket>
#include <QUrl>

#include <QJsonObject>
#include <QJsonDocument>

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
const int VALMAX_TEMP = 30;
const int VALMIN_TEMP = 15;
const int VALMAX_WNDSPD = 3;
const int VALMIN_WNDSPD = 0;
const int INTERVAL_IMMEDIATE = 1;
const int INTERVAL_RECONNECT = 3000;
const int INTERVAL_SYNCCONTROLINFO = 3000;
const int INTERVAL_GETPRICE = 30000;

#define DEBUG
#ifdef DEBUG
    #define DEBUG_CONNECTION
    #define DEBUG_TIMER
#endif

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class Room : public QMainWindow
{
    Q_OBJECT

public:
    Room(QString ip, int port, QString room, QWidget *parent = nullptr);
    ~Room();

private:
    QTimer* timerReconnect;
    QTimer* timerSendControlInfo;
    QTimer* timerGetPrice;

public:
    Ui::MainWindow *ui;

private:
    QWebSocket* sock;
    bool isConnected;
    bool isControlInfoEditted;

    QString svRoomID;
    QString svUserID;

private:
    QJsonObject string2jsonobj(const QString& str);
    QString jsonobj2string(const QJsonObject& obj);

private:
    void rcvType1(const QJsonObject& json);
    void rcvType2(const QJsonObject& json);

private:
    void connectSrv(QString, int);
    void onConnected();
    void onDisconnect();

    void sendMsg(int);
    void onMsgRcv(const QString&);

    void updateUI();
};
#endif // ROOM_H
