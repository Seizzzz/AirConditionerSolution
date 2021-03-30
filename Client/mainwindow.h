#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QUdpSocket>
#include <QJsonObject>
#include <QJsonDocument>

#define JSONAME_TYPE "type"
#define JSONAME_ROOMID "roomID"
#define JSONAME_USERID "userID"
#define JSONAME_WNDSPD "wind_speed"
#define JSONAME_TEMP "tepture"
#define JSONAME_MODE "model"

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
    Ui::MainWindow *ui;

    QUdpSocket* sock;

    int PORT_SRV = 62100;
    char ADDR_SRV[32] = "127.0.0.1";

    void syncServer(int);
    void syncLocal();
};
#endif // MAINWINDOW_H
