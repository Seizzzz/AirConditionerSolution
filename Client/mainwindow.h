#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>

#include <QWebSocket>
#include <QUrl>

#include <QJsonObject>
#include <QJsonDocument>

#define JSONAME_TYPE "type"
#define JSONAME_ROOMID "roomID"
#define JSONAME_USERID "userID"
#define JSONAME_WNDSPD "wind_speed"
#define JSONAME_TEMP "tepture"
#define JSONAME_MODE "model"

#define SRV_ADDR "127.0.0.1"
#define SRV_PORT "62100"

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
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    QTimer* timerReconnect;

private:
    Ui::MainWindow *ui;

    QWebSocket* sock;
    bool isConnected;

    void onConnected();
    void onDisconnect();
    void onMsgRcv(const QString&);

    void connectSrv();
    void syncServer(int);
    void syncLocal();
};
#endif // MAINWINDOW_H
