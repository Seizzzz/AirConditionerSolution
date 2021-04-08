#ifndef MAINWINDOW_H
#define MAINWINDOW_H

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

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QString ip, int port, QString room, QWidget *parent = nullptr);
    ~MainWindow();

private:
    QTimer* timerReconnect;
    QTimer* timerSendControlInfo;
    QTimer* timerGetPrice;

private:
    Ui::MainWindow *ui;

    QWebSocket* sock;
    bool isConnected;
    bool isControlInfoEditted;

    QString svRoomID;

private:
    QJsonObject string2jsonobj(const QString& str);
    QString jsonobj2string(const QJsonObject& obj);

private:
    void onConnected();
    void onDisconnect();
    void onMsgRcv(const QString&);

    void connectSrv(QString, int);
    void syncServer(int);
    void syncLocal();

    void updateUI();
};
#endif // MAINWINDOW_H
