#include "console.h"

#include <QDebug>

inline QString getIdentifier(QWebSocket* peer)
{
    return QStringLiteral("%1 %2").arg(peer->peerAddress().toString()).arg(QString::number(peer->peerPort()));
}

inline QJsonObject Console::string2jsonobj(const QString& str)
{
    return QJsonDocument::fromJson(str.toLocal8Bit().data()).object();
}

inline QString Console::jsonobj2string(const QJsonObject& obj)
{
    return QString(QJsonDocument(obj).toJson());
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
    QString idt = getIdentifier(sockClt);

#ifdef DEBUG_DISCONNECTED
    qDebug() << idt << "disconnected";
#endif
    if(sockClt)
    {
        lstClt.removeAll(sockClt);
        sockClt->deleteLater();

        //移除对应关系，为后续备用
        if(sockInfo.find(idt) != sockInfo.end()) sockInfo.remove(idt);
    }
}

int Console::getPriceCost(const QJsonObject& json){
    return rand();

    QSqlQuery query(db);
    int money = 0;
    QString str = QString("select * from room where RoomId = '%1' and UserId = '%2' ordered by Time")
            .arg(json[JSONAME_ROOMID].toInt())
            .arg(json[JSONAME_USERID].toInt());
    query.exec(str);
    while(query.next()){
        money++;
    }
    return money;
}

//房间开关机
QString Console::ProcessType0(const QJsonObject& json)
{
    QJsonObject msg;
    msg[JSONAME_TYPE] = 0;
    int isPowerOn = msg[JSONAME_POWER].toInt();

    //插入开关的控制信息
    QSqlQuery query(db);
    int timeStamp = QDateTime::currentSecsSinceEpoch();
    QString stmt = QString("insert into opt values(%1,%2,%3,%4,%5,%6,%7)")
            .arg(json[JSONAME_ROOMID].toInt())
            .arg(json[JSONAME_USERID].toInt())
            .arg("NULL")
            .arg("NULL")
            .arg("NULL")
            .arg(isPowerOn)
            .arg(timeStamp);
    bool execed = query.exec(stmt);
#ifdef DEBUG_DB_QUERY
    qDebug() << "query exec: " << stmt;
    //qDebug() << query.lastError();
    qDebug() << "suc execed: " << execed;
#endif

    if(execed) msg[JSONAME_ACK] = true;
    else msg[JSONAME_ACK] = false;

    auto jsonString = jsonobj2string(msg);
    return jsonString;
}

//房间获取userID
QString Console::ProcessType1(const QJsonObject& json, QString& idt)
{
    QJsonObject msg;
    msg[JSONAME_TYPE] = 1;

    QString roomID = json[JSONAME_ROOMID].toString();

    QSqlQuery query(db);
    QString str = QString("select userId from room where roomId = '%1'")
            .arg(roomID);
    query.exec(str);

    if(query.next())
    {
        QString userID = query.value(0).toString();
        msg[JSONAME_ROOMID] = roomID;
        msg[JSONAME_USERID] = userID;

        //保存一个连接与rid和uid的映射关系，为后续功能备用
        sockInfo.insert(idt, userInfo(roomID, userID));
    }
    else qDebug() << "room access denied";

    auto jsonString = jsonobj2string(msg);
    return jsonString;
}

//房间改变控制信息
QString Console::ProcessType2(const QJsonObject& json)
{
    QJsonObject msg;
    msg[JSONAME_TYPE] = 2;

    auto jsonAirData = string2jsonobj(json[JSONAME_AIRDATA].toString());
    //判断控制信息改变是否合法
    int wantedTemp = jsonAirData[JSONAME_TEMP].toInt();
    int wantedWndSpd = jsonAirData[JSONAME_WNDSPD].toInt();
    if(wantedTemp > 30 || wantedTemp < 15 || wantedWndSpd > 3 || wantedWndSpd < 0)
        msg[JSONAME_ACK] = false;
    else
    {
        msg[JSONAME_ACK] = true;

        //将合法的控制信息保存到数据库中
        QSqlQuery query(db);
        int timeStamp = QDateTime::currentSecsSinceEpoch();
        QString stmt = QString("insert into opt values(%1,%2,%3,%4,%5,%6,%7)")
                .arg(json[JSONAME_ROOMID].toInt())
                .arg(json[JSONAME_USERID].toInt())
                .arg(jsonAirData[JSONAME_WNDSPD].toInt())
                .arg(jsonAirData[JSONAME_TEMP].toInt())
                .arg(jsonAirData[JSONAME_MODE].toInt())
                .arg(1)
                .arg(timeStamp);
        bool execed = query.exec(stmt);
#ifdef DEBUG_DB_QUERY
        qDebug() << "query exec: " << stmt;
        //qDebug() << query.lastError();
        qDebug() << "suc execed: " << execed;
#endif
    }

    //获取金额
    int money = getPriceCost(json);
    msg[JSONAME_MONEY] = money;

    auto jsonString = jsonobj2string(msg);
    return jsonString;
}

//前台开房
QString Console::ProcessType3(const QJsonObject& json)
{
    QJsonObject msg;
    msg[JSONAME_TYPE] = 3;

    //获取所有空房间
    QSqlQuery query(db);
    QString str = QString("select Roomid from roomstate where State = '%1'").arg(1);
    query.exec(str);

    int roomId = -1;
    if(query.next())
    {
        //选择查询到的第一个空房间
        roomId = query.value(0).toInt();

        //更新房间状态
        str = QString("updata from roomstate set State = '%1' where RoomId = '%2'")
                .arg(0)
                .arg(roomId);
        query.exec(str);

        //插入房间信息
        int timeStamp = QDateTime::currentSecsSinceEpoch();
        str = QString("insert into room values('%1','%2','%3','%4','%5','%6')")
                .arg(roomId)
                .arg(json[JSONAME_USERID].toInt())
                .arg(0)
                .arg(0)
                .arg(0)
                .arg(timeStamp);
        query.exec(str);
    }

    //获取到了房间
    if(roomId != -1)
    {
        msg[JSONAME_ROOMID] = roomId;
        msg[JSONAME_USERID] = json[JSONAME_USERID];
    }
    else qDebug() << "no free room";

    auto jsonString = jsonobj2string(msg);
    return jsonString;
}


//退房、获取详单
QString Console::ProcessType4(const QJsonObject& json){
    QJsonObject msg;
    msg[JSONAME_TYPE] = 4;

    QSqlQuery query(db);

    //更新房间空闲状态
    QString str = QString("update from roomstate set State = '%1' where RoomId = '%2'")
            .arg(1)
            .arg(json[JSONAME_ROOMID].toInt());
    query.exec(str);

    if(query.next()) { //退房成功
        //更新房间信息
        int timeStamp = QDateTime::currentSecsSinceEpoch();
        str = QString("insert into room values('%1','%2','%3','%4','%5','%6')")
                .arg(json[JSONAME_ROOMID].toInt())
                .arg(json[JSONAME_USERID].toInt())
                .arg(0)
                .arg(0)
                .arg(0)
                .arg(timeStamp);
        query.exec(str);
    }

    str = QString("select * from room where RoomId = '%1' and UserId = '%2'"
                  ).arg(json[JSONAME_ROOMID].toInt()).arg(json[JSONAME_USERID].toInt());
    query.exec(str);
    while(query.next()){
        //详单信息获取
    }

    auto jsonString = jsonobj2string(msg);
    return jsonString;
}

QString Console::ProcessType5(const QJsonObject& json){
    QJsonObject msg;
    msg[JSONAME_TYPE] = 5;

    qDebug() << json.empty();

    auto jsonString = jsonobj2string(msg);
    return jsonString;
}
QString Console::ProcessType6(const QJsonObject& json){
    QJsonObject msg;
    msg[JSONAME_TYPE] = 6;

    qDebug() << json.empty();

    auto jsonString = jsonobj2string(msg);
    return jsonString;
}
QString Console::ProcessType7(const QJsonObject& json){
    QJsonObject msg;
    msg[JSONAME_TYPE] = 7;

    qDebug() << json.empty();

    auto jsonString = jsonobj2string(msg);
    return jsonString;
}
QString Console::ProcessType8(const QJsonObject& json){
    QJsonObject msg;
    msg[JSONAME_TYPE] = 8;

    qDebug() << json.empty();

    auto jsonString = jsonobj2string(msg);
    return jsonString;
}

void Console::process(const QString& msg)
{
    QJsonObject json = string2jsonobj(msg);
    QWebSocket* clt = qobject_cast<QWebSocket*>(sender());
    QString ident = getIdentifier(clt);

#ifdef DEBUG_RCV_CONTENT
    qDebug() << "rcv: " << msg;
#endif

    //按type域分类处理
    int opt = json[JSONAME_TYPE].toInt();
    QString jsonRet;
    switch(opt)
    {
    case 1:
        jsonRet = ProcessType1(json, ident);
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

#ifdef DEBUG_SEND_CONTENT
    qDebug() << "send: " << msg;
#endif
    clt->sendTextMessage(jsonRet);
}

void Console::connectMySQL()
{
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
