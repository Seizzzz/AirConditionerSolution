#include "console.h"

#include <QDebug>

static QString getIdentifier(QWebSocket* peer)
{
    return QStringLiteral("%1 %2").arg(peer->peerAddress().toString(), QString::number(peer->peerPort()));
}

void Console::onNewConnection()
{
    auto sockClt = sock->nextPendingConnection();
#ifdef DEBUG_CONNECTED
    qDebug() << getIdentifier(sockClt) << "connected";
#endif
    sockClt->setParent(this);

    connect(sockClt, &QWebSocket::textMessageReceived, this, &Console::process);
    connect(sockClt, &QWebSocket::disconnected, this, &Console::onDisconnect);

    lstClt << sockClt;
}

void Console::onDisconnect()
{
    QWebSocket* sockClt = qobject_cast<QWebSocket*>(sender());
#ifdef DEBUG_DISCONNECTED
    qDebug() << getIdentifier(sockClt) << "disconnected";
#endif
    if(sockClt)
    {
        lstClt.removeAll(sockClt);
        sockClt->deleteLater();
    }
}

void Console::process(const QString& msg)
{
    //restore json
    QWebSocket* clt = qobject_cast<QWebSocket*>(sender());

#ifdef DEBUG_RCV_CONTENT
    qDebug() << msg;
#endif
    QJsonDocument jsonDoc = QJsonDocument::fromJson(msg.toLocal8Bit().data());
    QJsonObject json = jsonDoc.object();

    //process
    int opt = json[JSONAME_TYPE].toInt();
    switch(opt)
    {
    default:

        qDebug() << opt;
    }

    //ret
    clt->sendTextMessage(msg);
}

void Console::connectMySQL()
{
    if(QSqlDatabase::contains("connection"))
    {
        db = QSqlDatabase::database("connection");
    }
    else
    {
        db = QSqlDatabase::addDatabase("QODBC", "connection");
        db.setHostName(DB_ADDR);
        db.setPort(DB_PORT);
        db.setDatabaseName(DB_DATABASE_NAME);

        QTextStream qin(stdin);
        QString buf;
        qin >> buf;
        db.setUserName(buf);
        qin >> buf;
        db.setPassword(buf);
    }

    if(!db.open())
    {
#ifdef DEBUG_DB_ERR
        qDebug() << "failed to connect db";
#endif
    }

}

Console::Console(quint16 port, QObject* parent) :
    QObject(parent),
    sock(new QWebSocketServer(QStringLiteral("Server"),
                              QWebSocketServer::NonSecureMode,
                              this))
{
    if(sock->listen(QHostAddress::Any, port))
    {
        connect(sock, &QWebSocketServer::newConnection, this, &Console::onNewConnection);
    }

    connectMySQL();
}

Console::~Console()
{
    sock->close();
    db.close();
}
