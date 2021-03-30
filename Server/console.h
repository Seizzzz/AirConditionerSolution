#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <QUdpSocket>
#include <QJsonObject>
#include <QJsonDocument>

#define DEBUG_RCV_PEER
#define DEBUG_RCV_CONTENT

class Console : public QObject
{
    Q_OBJECT

public:

    QUdpSocket* sock;

    Console();

    void sockListen();

};


#endif // SERVER_H
