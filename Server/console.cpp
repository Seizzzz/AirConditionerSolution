#include "console.h"

#include <iostream>
#include <QDebug>

void Console::sockListen()
{
    QHostAddress addrClt;
    quint16 portClt;

    //rcv
    QByteArray buf;
    buf.resize(sock->pendingDatagramSize());
    sock->readDatagram(buf.data(), buf.size(), &addrClt, &portClt);

    const QString jsonString = QString(buf.data());

#ifdef DEBUG_RCV_CONTENT
    qDebug() << jsonString;
#endif

    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonString.toLocal8Bit().data());
    QJsonObject json = jsonDoc.object();

    //reply
#ifdef DEBUG_RCV_PEER
    qDebug() << sock->peerAddress() << sock->peerPort();
#endif
    sock->writeDatagram(buf, addrClt, portClt);
}

Console::Console()
{
    sock = new QUdpSocket(this);
    sock->bind(QHostAddress::LocalHost, 62100);

    connect(sock, &QUdpSocket::readyRead, this, &Console::sockListen);
}
