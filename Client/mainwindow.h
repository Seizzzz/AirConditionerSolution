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

#define DEBUG
#ifdef DEBUG
    #define DEBUG_CONNECTION
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
    QString svRoomID;

private:
    Ui::MainWindow *ui;

    QWebSocket* sock;
    bool isConnected;

    void onConnected();
    void onDisconnect();
    void onMsgRcv(const QString&);

    void connectSrv(QString, int);
    void syncServer(int);
    void syncLocal();
};
#endif // MAINWINDOW_H
