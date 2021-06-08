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
    int defaultTemp;
    int maxTemp;
    int minTemp;
    double highFee;
    double midFee;
    double lowFee;
};

#endif // ADMIN_H
