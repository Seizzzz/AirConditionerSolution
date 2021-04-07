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
void Console::ProcessType1(QJsonObject json, QWebSocket* clt){
    QSqlQuery query(db);
    time = QDateTime::currentDateTime();   //get current time
    int timeT = time.toTime_t();
    QString str = QString("insert into room values(%1,'%2','%3','%4','%5','%6')"
                          ).arg(json[JSONAME_ROOMID].toInt()).arg(json[JSONAME_USERID].toString())
            .arg(json[JSONAME_WNDSPD].toInt()).arg(json[JSONAME_TEMP].toInt()).arg(1).arg(timeT);
    query.exec(str);
    QJsonObject mesg; //send mesg to client
    if(query.next()) { //updata successful
        mesg[JSONAME_TYPE] = 1;
        mesg[JSONAME_ACK] = true;
        //add other information
    }
    auto jsonString = QString(QJsonDocument(mesg).toJson());
    clt->sendTextMessage(jsonString);
#ifdef DEBUG_SEND_CONTENT
    qDebug() << "1234\n";
#endif
}

//int Console::ComputeMoney(QJsonObject json){
//    QSqlQuery query(db);
//    int money = 0;
//    QString str = QString("select * from room where RoomId = '%1' and UserId = '%2' ordered by Time"
//                          ).arg(json[JSONAME_ROOMID].toInt()).arg(json[JSONAME_USERID].toInt());
//    query.exec(str);
//    while(query.next()){
//        money++;
//        //按照时间升序，计算每个时间段的金额然后相加
//    }
//}

int Console::ComputeMoney(QJsonObject json){
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

void Console::ProcessType2(QJsonObject json, QWebSocket* clt){
   int money = ComputeMoney(json);
   QJsonObject mesg;
   mesg[JSONAME_TYPE] = 2;
   mesg[JSONAME_MONEY] = money;
   auto jsonString = QString(QJsonDocument(mesg).toJson());
   clt->sendTextMessage(jsonString);
}

//get a free room's RoomId
void Console::ProcessType3(QJsonObject json, QWebSocket* clt){
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
    clt->sendTextMessage(jsonString);
}


//check out
void Console::ProcessType4(QJsonObject json){
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
void Console::ProcessType5(QJsonObject json){

}
void Console::ProcessType6(QJsonObject json){

}
void Console::ProcessType7(QJsonObject json){

}
void Console::ProcessType8(QJsonObject json){

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
    switch(opt)
    {
    case 1:
        ProcessType1(json, clt);
        break;
    case 2:
        ProcessType2(json,clt);
        break;
    case 3:
        ProcessType3(json, clt);
        break;
    case 4:
        ProcessType4(json);
        break;
    case 5:
        ProcessType5(json);
        break;
    case 6:
        ProcessType6(json);
        break;
    case 7:
        ProcessType7(json);
        break;
    case 8:
        ProcessType8(json);
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
    clt->sendTextMessage(msg);
}

void Console::connectMySQL()
{
//    if(QSqlDatabase::contains("connection"))
//    {
//        db = QSqlDatabase::database("connection");
//    }
//    else
//    {
    db = QSqlDatabase::addDatabase("QMYSQL");
    db.setHostName(DB_ADDR);
    db.setPort(DB_PORT);
    db.setDatabaseName(DB_DATABASE_NAME);
    QTextStream qin(stdin);
    QString buf;
    //qin >> buf;
    db.setUserName("root");
    //qin >> buf;
    db.setPassword("fuckyou");
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
