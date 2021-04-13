#ifndef RECEPTION_H
#define RECEPTION_H

#include <QWidget>

#include <QWebSocket>

#include <QJsonObject>
#include <QJsonDocument>

namespace Ui {
class Reception;
}

class Reception : public QWidget
{
    Q_OBJECT

public:
    Reception(QString ip, int port, QWidget *parent = nullptr);
    ~Reception();

private:
    Ui::Reception *ui;

    void connectSrv(QString, int);
    QWebSocket* sock;

    void onConnected();
    void onDisconnect();
    void onMsgRcv(const QString&);

private:
    QJsonObject string2jsonobj(const QString& str);
    QString jsonobj2string(const QJsonObject& obj);
};

#endif // RECEPTION_H
