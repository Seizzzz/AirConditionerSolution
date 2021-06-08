#ifndef ROOM_H
#define ROOM_H

#include <QWidget>

#include <QTimer>
#include <QString>

#include <QWebSocket>
#include <QUrl>

#include <QJsonObject>
#include <QJsonDocument>

#include "../tools.h"

#define DEBUG
#ifdef DEBUG
    #define DEBUG_RCV_CONTENT
    #define DEBUG_CONNECTION
    #define DEBUG_TIMER
#endif

namespace Ui {
class Room;
}

class Room : public QWidget
{
    Q_OBJECT

public:
    Room(QString ip, int port, QString room, QWidget *parent = nullptr);
    ~Room();

private:
    Ui::Room *ui;

private:
    QTimer* timerReconnect;
    QTimer* timerSendControlInfo;
    QTimer* timerGetPrice;

private:
    QWebSocket* sock;
    bool isConnected;

    QString svRoomID;
    QString svUserID;

private:
    void rcvType0(const QJsonObject& json);
    void rcvType1(const QJsonObject& json);
    void rcvType2(const QJsonObject& json);
    void rcvType3(const QJsonObject& json);
    void rcvType4(const QJsonObject& json);
    void rcvType13(const QJsonObject& json);

private:
    void connectSrv(QString, int);
    void onConnected();
    void onDisconnect();

    void sendMsg(int);
    void onMsgRcv(const QString&);

    void updateUI();

private:
    void reqEnter();
};

#endif // ROOM_H
