#include "console.h"

#include <QDebug>

static QString getIdentifier(QWebSocket* peer)
{
    return QStringLiteral("%1 %2").arg(peer->peerAddress().toString(), QString::number(peer->peerPort()));
}

void Console::onNewConnection()
{
    auto sockClt = sock->nextPendingConnection();
    if(!sockClt->isValid()) return;
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
QString Console::ProcessType1(const QJsonObject& json)
{
    auto stringAirData = json[JSONAME_AIRDATA].toString();
    QJsonObject jsonAirData = QJsonDocument::fromJson(stringAirData.toLocal8Bit().data()).object();

    int wantedTemp = jsonAirData[JSONAME_TEMP].toInt();
    int wantedWndSpd = jsonAirData[JSONAME_WNDSPD].toInt();

    qDebug() << wantedTemp << wantedWndSpd;

    if(wantedTemp > 30 || wantedTemp < 15 || wantedWndSpd > 3 || wantedWndSpd < 0)
    {
        QJsonObject msg;
        msg[JSONAME_TYPE] = 1;
        msg[JSONAME_ACK] = false;

        auto jsonString = QString(QJsonDocument(msg).toJson());
        return jsonString;
    }

    QSqlQuery query(db);
    time = QDateTime::currentDateTime();   //get current time
    int timeT = time.toTime_t();
    QString stmt = QString("insert into opt values(%1,%2,%3,%4,%5,%6,%7)"
                          ).arg(json[JSONAME_ROOMID].toInt()).arg(json[JSONAME_USERID].toInt())
            .arg(jsonAirData[JSONAME_WNDSPD].toInt()).arg(jsonAirData[JSONAME_TEMP].toInt()).arg(jsonAirData[JSONAME_MODE].toInt()).arg(1).arg(timeT);
    bool execed = query.exec(stmt);

#ifdef DEBUG_DB_QUERY
        qDebug() << "query exec: " << stmt;
        //qDebug() << query.lastError();
        qDebug() << "suc execed: " << execed;
#endif

    QJsonObject mesg; //send mesg to client
    if(query.next()) { //updata successful
        mesg[JSONAME_TYPE] = 1;
        mesg[JSONAME_ACK] = true;
        //add other information
    }
    auto jsonString = QString(QJsonDocument(mesg).toJson());

    return jsonString;
#ifdef DEBUG_SEND_CONTENT
    qDebug() << "1234\n";
#endif
}

int Console::ComputeMoney(const QJsonObject& json){
    QSqlQuery query(db);
    int money = 0;
    QString str = QString("select * from room where RoomId = '%1' and UserId = '%2' ordered by Time"
                          ).arg(json[JSONAME_ROOMID].toInt()).arg(json[JSONAME_USERID].toInt());
    query.exec(str);
    while(query.next()){
        money++;
    }
    return money;
}

QString Console::ProcessType2(const QJsonObject& json){
    int money = ComputeMoney(json);
    QJsonObject mesg;
    mesg[JSONAME_TYPE] = 2;
    mesg[JSONAME_MONEY] = money;
    auto jsonString = QString(QJsonDocument(mesg).toJson());

    return jsonString;
}

//get a free room's RoomId
QString Console::ProcessType3(const QJsonObject& json){
    QSqlQuery query(db);
    QString str = QString("select Roomid from roomstate where State = '%1'").arg(1);
    query.exec(str);
    int roomId = -1;
    if(query.next()){//if we have free rooms
        roomId = query.value(0).toInt(); //return free roomID
        //updata roomstate table
        str = QString("updata from roomstate set State = '%1' where RoomId = '%2'"
                      ).arg(0).arg(roomId);
        query.exec(str);
        //updata room table
        QDateTime time = QDateTime::currentDateTime();   //get current time
        int timeT = time.toTime_t();   //将当前时间转为时间戳
        str = QString("insert into room values('%1','%2','%3','%4','%5','%6')"
                      ).arg(roomId).arg(json[JSONAME_USERID].toInt()).arg(0).arg(0).arg(0).arg(timeT);
        query.exec(str);
    }
    QJsonObject mesg; //send mesg to front desk
    if(roomId) {
        mesg[JSONAME_TYPE] = 3;
        mesg[JSONAME_ROOMID] = roomId;
        mesg[JSONAME_USERID] = json[JSONAME_USERID];
    }
    //if(!roomId) qDebug << "no free room"; //no free rooms
    auto jsonString = QString(QJsonDocument(mesg).toJson());

    return jsonString;
}


//check out
QString Console::ProcessType4(const QJsonObject& json){
    QSqlQuery query(db);
    QString str = QString("updata from roomstate set State = '%1' where RoomId = '%2'"
                          ).arg(1).arg(json[JSONAME_ROOMID].toInt());
    query.exec(str);  //updata roomstate table
    if(query.next()) { //退房成功
        //updata room table
        time = QDateTime::currentDateTime();   //get current time
        int timeT = time.toTime_t();   //将当前时间转为时间戳
        str = QString("insert into room values('%1','%2','%3','%4','%5','%6')"
                      ).arg(json[JSONAME_ROOMID].toInt()).arg(json[JSONAME_USERID].toInt()).arg(0).arg(0).arg(0).arg(timeT);
        query.exec(str);//end updata room table
    }
    QJsonObject mesg; //send mesg to front desk
    str = QString("select * from room where RoomId = '%1' and UserId = '%2'"
                  ).arg(json[JSONAME_ROOMID].toInt()).arg(json[JSONAME_USERID].toInt());
    query.exec(str);
    while(query.next()){
        //详单信息获取
    }
}
QString Console::ProcessType5(const QJsonObject& json){

}
QString Console::ProcessType6(const QJsonObject& json){

}
QString Console::ProcessType7(const QJsonObject& json){

}
QString Console::ProcessType8(const QJsonObject& json){

}

void Console::process(const QString& msg)
{
    //restore json
    QWebSocket* clt = qobject_cast<QWebSocket*>(sender());

#ifdef DEBUG_RCV_CONTENT
    qDebug() << "rcv: " << msg;
#endif
    QJsonDocument jsonDoc = QJsonDocument::fromJson(msg.toLocal8Bit().data());
    QJsonObject json = jsonDoc.object();
    //process
    int opt = json[JSONAME_TYPE].toInt();
    QString jsonRet;
    switch(opt)
    {
    case 1:
        jsonRet = ProcessType1(json);
        break;
    case 2:
        jsonRet = ProcessType2(json);
        break;
    case 3:
        jsonRet = ProcessType3(json);
        break;
    case 4:
        jsonRet = ProcessType4(json);
        break;
    case 5:
        jsonRet = ProcessType5(json);
        break;
    case 6:
        jsonRet = ProcessType6(json);
        break;
    case 7:
        jsonRet = ProcessType7(json);
        break;
    case 8:
        jsonRet = ProcessType8(json);
        break;
    default:
        int roomid = json[JSONAME_ROOMID].toInt();
        int wndspd = json[JSONAME_WNDSPD].toInt();
        int temp = json[JSONAME_TEMP].toInt();
        QSqlQuery query(db);
        auto stmt = QString("insert into room values(%1, null, %2, null, %3, %4)").arg(roomid).arg(opt).arg(wndspd).arg(temp);
        bool execed = query.exec(stmt);

#ifdef DEBUG_DB_QUERY
        qDebug() << "query exec: " << stmt;
        //qDebug() << query.lastError();
        qDebug() << "suc execed: " << execed;
#endif
    }
    //ret
//#ifdef DEBUG_SEND_CONTENT
//    qDebug() << "send: " << msg;
//#endif
    clt->sendTextMessage(jsonRet);
}

void Console::connectMySQL()
{
//    if(QSqlDatabase::contains("connection"))
//    {
//        db = QSqlDatabase::database("connection");
//    }
//    else
//    {
    db = QSqlDatabase::addDatabase("QODBC");
    db.setHostName(DB_ADDR);
    db.setPort(DB_PORT);
    db.setDatabaseName(DB_DATABASE_NAME);
    QTextStream qin(stdin);
    QString buf;
    //qin >> buf;
    db.setUserName("root");
    //qin >> buf;
    db.setPassword("Battle126");
//    }

    if(!db.open())
    {
#ifdef DEBUG_DB_ERR
        //qDebug() << "Failed connect to db";
        qDebug() << db.lastError().text();
#endif
    }
    else
    {
#ifdef DEBUG_DB_ERR
        qDebug() << "connected db";
#endif
    }

}

Console::Console(quint16 port, QObject* parent) :
    QObject(parent),
    sock(new QWebSocketServer(QStringLiteral("Server"),
                              QWebSocketServer::NonSecureMode,
                              this))
{
    //sock
    if(sock->listen(QHostAddress::Any, port))
    {
        connect(sock, &QWebSocketServer::newConnection, this, &Console::onNewConnection);
    }

    //db
    connectMySQL();
}

Console::~Console()
{
    sock->close();
    db.close();
}
