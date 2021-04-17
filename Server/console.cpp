#include "console.h"

#include <QDebug>

inline QString getIdentifier(QWebSocket* peer)
{
    return QStringLiteral("%1:%2").arg(peer->peerAddress().toString()).arg(QString::number(peer->peerPort()));
}

//inline QJsonObject Console::string2jsonobj(const QString& str)
//{
//    //return QJsonDocument::fromJson(str.toLocal8Bit().data()).object();

//    return QJsonDocument::fromJson(str.toUtf8()).object();
//}

//inline QString Console::jsonobj2string(const QJsonObject& obj)
//{
//    //return QString(QJsonDocument(obj).toJson());

//    return QString(QJsonDocument(obj).toJson(QJsonDocument::Compact));
//}

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

//从数据库查询指定roomID、userID的金额
double Console::getPriceCost(const QJsonObject& json){
    QSqlQuery query(db);

    double money = 0;
    QString str = QString("select * from %1 where %2 = %3 and %4 = '%5'")
            .arg(DBNAME_TABLE_ROOM)
            .arg(DBNAME_FIELD_RID)
            .arg(json[JSONAME_ROOMID].toString())
            .arg(DBNAME_FIELD_UID)
            .arg(json[JSONAME_USERID].toString());

#ifdef DEBUG_DB_QUERY
    bool execed = query.exec(str);
    qDebug()<< "id and userid" << json[JSONAME_ROOMID] << json[JSONAME_USERID];
    qDebug() << "query exec: " << str;
    //qDebug() << query.lastError();
    qDebug() << "suc execed: " << execed;
#endif

    int subtime = 0, temp_time;
    int airstate;
    for(int i = 0; query.next(); i++, subtime = 0) {
        int tempture = query.value(3).toInt();
        int wind = query.value(2).toInt();
        if(i == 0) {
            temp_time = query.value(5).toInt();
            airstate = query.value(4).toInt();
        }
        else if((airstate == 0 && query.value(4).toInt() == 0)
                || (airstate == 0 && query.value(4).toInt() == 1))
        {
            subtime = 0;
        }
        else subtime = query.value(5).toInt() - temp_time;

        money += (abs(tempture - 23) + abs(wind - 3))*subtime;
        temp_time = query.value(5).toInt();
        airstate = query.value(4).toInt();
    }

    return money;
}

//房间开关机
QString Console::ProcessType0(const QJsonObject& json)
{
    QJsonObject msg;
    msg[JSONAME_TYPE] = 0;

    int isPowerOn = json[JSONAME_POWER].toInt();

    //插入开关的控制信息
    QSqlQuery query(db);
    int timeStamp = QDateTime::currentSecsSinceEpoch();
    QString stmt = QString("insert into %1 values('%2', '%3', %4, %5, %6, %7, %8)")
            .arg(DBNAME_TABLE_ROOM)
            .arg(json[JSONAME_ROOMID].toString())
            .arg(json[JSONAME_USERID].toString())
            .arg("NULL")
            .arg("NULL")
            .arg(isPowerOn)
            .arg(timeStamp)
            .arg("NULL");
    bool execed = query.exec(stmt);
#ifdef DEBUG_DB_QUERY
    qDebug() << "query exec: " << stmt;
    //qDebug() << query.lastError();
    qDebug() << "suc execed: " << execed;
#endif

    //todo
    msg[JSONAME_ACK] = true;

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
    QString stmt = QString("select %1 from %2 where %3 = '%4' order by %5 desc")
            .arg(DBNAME_FIELD_UID)
            .arg(DBNAME_TABLE_ROOM)
            .arg(DBNAME_FIELD_RID)
            .arg(roomID)
            .arg(DBNAME_FIELD_TIME);
    bool execed = query.exec(stmt);
#ifdef DEBUG_DB_QUERY
    qDebug() << "query exec: " << stmt;
    //qDebug() << query.lastError();
    qDebug() << "suc execed: " << execed;
#endif

    if(query.next())
    {
        QString userID = query.value(0).toString();
        msg[JSONAME_ROOMID] = roomID;
        msg[JSONAME_USERID] = userID;

        //保存一个连接与rid和uid的映射关系，为后续功能备用
        sockInfo.insert(idt, userInfo(roomID, userID));
    }
    else qDebug() << "room access denied"; //没有查询到userid

    auto jsonString = jsonobj2string(msg);
    return jsonString;
}

//房间改变控制信息
QString Console::ProcessType2(const QJsonObject& json)
{
    QJsonObject msg;
    msg[JSONAME_TYPE] = 2;

    auto jsonAirData = json[JSONAME_AIRDATA];
    //判断控制信息改变是否合法
    int wantedTemp = jsonAirData[JSONAME_TEMP].toInt();
    int wantedWndSpd = jsonAirData[JSONAME_WNDSPD].toInt();
    if(wantedTemp > 30 || wantedTemp < 15 || wantedWndSpd > 3 || wantedWndSpd < 0);
        //msg[JSONAME_ACK] = false;
    else
    {
        //msg[JSONAME_ACK] = true;

        //将合法的控制信息保存到数据库中
        QSqlQuery query(db);
        int timeStamp = QDateTime::currentSecsSinceEpoch();
        QString stmt = QString("insert into %1 values('%2', '%3', %4, %5, %6, %7, %8)")
                .arg(DBNAME_TABLE_ROOM)
                .arg(json[JSONAME_ROOMID].toString())
                .arg(json[JSONAME_USERID].toString())
                .arg(jsonAirData[JSONAME_WNDSPD].toInt())
                .arg(jsonAirData[JSONAME_TEMP].toInt())
                .arg(json[JSONAME_POWER].toInt())
                .arg(timeStamp)
                .arg(jsonAirData[JSONAME_MODE].toInt());
        bool execed = query.exec(stmt);
#ifdef DEBUG_DB_QUERY
        qDebug() << "query exec: " << stmt;
        //qDebug() << query.lastError();
        qDebug() << "suc execed: " << execed;
#endif
    }

    //获取金额
    double money = getPriceCost(json);
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
    QString stmt = QString("select %1 from %2 where %3 = %4")
            .arg(DBNAME_FIELD_RID2)
            .arg(DBNAME_TABLE_ROOMSTATE)
            .arg(DBNAME_FIELD_STATE)
            .arg(1);
    bool execed = query.exec(stmt);
#ifdef DEBUG_DB_QUERY
    qDebug() << "query exec: " << stmt;
    //qDebug() << query.lastError();
    qDebug() << "suc execed: " << execed;
#endif

    QString roomID = "UNDEFINED";
    if(query.next())
    {
        //选择查询到的第一个空房间
        roomID = query.value(0).toString();

        //更新房间状态
        stmt = QString("update roomstate set State = %1 where RoomId = '%2'")
                .arg(0)
                .arg(roomID);
        bool execed = query.exec(stmt);
#ifdef DEBUG_DB_QUERY
        qDebug() << "query exec: " << stmt;
        //qDebug() << query.lastError();
        qDebug() << "suc execed: " << execed;
#endif

        //插入一条初始控制信息，顺带存储userID
        int timeStamp = QDateTime::currentSecsSinceEpoch();
        stmt = QString("insert into %1 values('%2', '%3', %4, %5, %6, %7, %8)")
                .arg(DBNAME_TABLE_ROOM)
                .arg(roomID)
                .arg(json[JSONAME_USERID].toString())
                .arg("NULL")
                .arg("NULL")
                .arg("NULL")
                .arg(timeStamp)
                .arg("NULL");
        execed = query.exec(stmt);
#ifdef DEBUG_DB_QUERY
        qDebug() << "query exec: " << stmt;
        //qDebug() << query.lastError();
        qDebug() << "suc execed: " << execed;
#endif
    }

    //获取到了房间
    if(roomID != "UNDEFINED")
    {
        msg[JSONAME_ROOMID] = roomID;
        msg[JSONAME_USERID] = json[JSONAME_USERID];
    }
    else
    {
        qDebug() << "no free room"; //没有获取到房间
    }

    auto jsonString = jsonobj2string(msg);
    return jsonString;
}


//退房、获取详单
QString Console::ProcessType4(const QJsonObject& json)
{
    QJsonObject msg;
    msg[JSONAME_TYPE] = 4;

    QSqlQuery query(db);

    //update roomstate
    QString str = QString("update roomstate set State = %1 where RoomId = %2")
            .arg(1)
            .arg(json[JSONAME_ROOMID].toString());
    bool execed = query.exec(str);
#ifdef DEBUG_DB_QUERY
    qDebug() << "query exec: " << str;
    //qDebug() << query.lastError();
    qDebug() << "suc execed: " << execed;
#endif
    if(execed) { //check out
        //update room table
        int timeStamp = QDateTime::currentSecsSinceEpoch();
        str = QString("insert into %1 values(%2, '%3', %4, %5, %6, %7, %8)")
                .arg(DBNAME_TABLE_ROOM)
                .arg(json[JSONAME_ROOMID].toString())
                .arg("000000000000")
                .arg(0)
                .arg(23)
                .arg(1)
                .arg(timeStamp)
                .arg(0);
        bool execed = query.exec(str);
        #ifdef DEBUG_DB_QUERY
            qDebug() << "query exec: " << str;
            //qDebug() << query.lastError();
            qDebug() << "suc execed: " << execed;
        #endif
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
    case 0:
        jsonRet = ProcessType0(json);
        break;
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
    db.setDatabaseName(DBNAME_DATABASE);
    QTextStream qin(stdin);
    QString buf;
    //qin >> buf;
    db.setUserName("root");
    //qin >> buf;
    db.setPassword("");

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
