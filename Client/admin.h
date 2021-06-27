#ifndef ADMIN_H
#define ADMIN_H

#include <QWidget>

#include <QWebSocket>

#include <QJsonObject>
#include <QJsonDocument>

#include "../tools.h"

namespace Ui {
class Admin;
}

class Admin : public QWidget
{
    Q_OBJECT

public:
    explicit Admin(QString ip, int port, QWidget *parent = nullptr);
    ~Admin();

private:
    Ui::Admin *ui;

private:
    void connectSrv(QString, int);
    QWebSocket* sock;

    void onConnected();
    void onDisconnect();
    void onMsgRcv(const QString&);

private:
    void getInfo(const QJsonObject&);

private:
    int mode = 0;
    int defaultTemp = 24;
    int defaultWndSpd = 2;
    int maxTemp = 30;
    int minTemp = 16;
    double highFee = 2;
    double midFee = 1.5;
    double lowFee = 1;
};

#endif // ADMIN_H
