#include "console.h"

#include <QDebug>

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

    mapLinkInfo.insert(getIdentifier(sockClt), LinkInfo());

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

        //移除对应关系
        if(mapLinkInfo.find(idt) != mapLinkInfo.end()) mapLinkInfo.remove(idt);
    }
}

bool Console::isRoomExisted(const QString& roomID)
{
    for(auto iter : mapLinkInfo)
        if(iter.roomID == roomID) return true;
    return false;
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

//空调开机
QString Console::rcvType0(const QJsonObject& json, InfoIter info)
{
    //从包中获取
    double roomtemp = json[JSONAME_ROOMTEMP].toDouble();

    //对服务端存储的信息进行修改
    info->roomTemp = roomtemp;

    //数据库相关
    QSqlQuery query(db);
    int timeStamp = QDateTime::currentSecsSinceEpoch();
    QString stmt = QString("insert into %1 values('%2', '%3', %4, %5, %6, %7, %8)")
            .arg(DBNAME_TABLE_ROOM)
            .arg(info->roomID)
            .arg(info->userID)
            .arg("NULL")
            .arg("NULL")
            .arg(1)
            .arg(timeStamp)
            .arg("NULL");
    bool execed = query.exec(stmt);
#ifdef DEBUG_DB_QUERY
    qDebug() << "query exec: " << stmt;
    qDebug() << "suc execed: " << execed;
#endif

    //返回包
    QJsonObject ret;
    ret[JSONAME_TYPE] = 0;
    ret[JSONAME_MODE] = 0;
    ret[JSONAME_WNDSPD] = 0;
    ret[JSONAME_DEFAULTEMP] = 0;
    ret[JSONAME_FEERATE] = 0;
    return jsonobj2string(ret);
}

//房间获取userID
QString Console::rcvType1(const QJsonObject& json, InfoIter info)
{
    //从包中获取
    QString roomID = json[JSONAME_ROOMID].toString();

    //检测是否已有该房间的连接、是否已开房
    bool suc = false;
    if(!isRoomExisted(roomID))
    {
        //数据库相关
        QSqlQuery query(db); //向数据库查询roomID对应的userID
        QString stmt = QString("select %1 from %2 where %3 = '%4' order by %5 desc")
                .arg(DBNAME_FIELD_UID)
                .arg(DBNAME_TABLE_ROOM)
                .arg(DBNAME_FIELD_RID)
                .arg(roomID)
                .arg(DBNAME_FIELD_TIME);
        bool execed = query.exec(stmt);
        #ifdef DEBUG_DB_QUERY
            qDebug() << "query exec: " << stmt;
            qDebug() << "suc execed: " << execed;
        #endif

        if(query.next())
        {
            suc = true;
            auto userID = query.value(0).toString();

            //对服务端存储的信息进行修改
            info->roomID = roomID;
            info->userID = userID;
        }
        else qDebug() << "Room not avaliable"; //没有查询到userid
    }

    //返回包
    QJsonObject msg;
    msg[JSONAME_TYPE] = 1;
    msg[JSONAME_ROOMID] = info->roomID;
    msg[JSONAME_USERID] = suc ? info->userID : "-1";
    return jsonobj2string(msg);
}

//房间改变风速
QString Console::rcvType2(const QJsonObject& json, InfoIter info)
{
    //从包中获取
    auto targetWndSpd = json[JSONAME_WNDSPD].toInt();

    //档位匹配费率
    double feerate = -1;
    switch (targetWndSpd) {
    default:
        feerate = 666;
    }

    //判断控制信息改变是否合法
    if(feerate != -1)
    {
        //对服务端存储的信息进行修改
        info->targetWndSpd = targetWndSpd;

        //将合法的控制信息保存到数据库中
        QSqlQuery query(db);
        int timeStamp = QDateTime::currentSecsSinceEpoch();
        QString stmt = QString("insert into %1 values('%2', '%3', %4, %5, %6, %7, %8)") //todo
                .arg(DBNAME_TABLE_ROOM)
                .arg(info->roomID)
                .arg(info->userID)
                .arg(info->targetWndSpd)
                .arg("NULL")
                .arg(1)
                .arg(timeStamp);
        bool execed = query.exec(stmt);
        #ifdef DEBUG_DB_QUERY
        qDebug() << "query exec: " << stmt;
        //qDebug() << query.lastError();
        qDebug() << "suc execed: " << execed;
        #endif
    }

    //返回包
    QJsonObject msg;
    msg[JSONAME_TYPE] = 2;
    msg[JSONAME_ACK] = 1;
    msg[JSONAME_FEERATE] = feerate;
    msg[JSONAME_WNDSPD] = info->targetWndSpd;
    return jsonobj2string(msg);
}

//房间改变温度
QString Console::rcvType3(const QJsonObject& json, InfoIter info)
{
    auto targetTemp = json[JSONAME_TEMP].toInt();

    if(true) //如果温度改变合法
    {
        info->targetTemp = targetTemp;
    }

    //返回包
    QJsonObject msg;
    msg[JSONAME_TYPE] = 3;
    msg[JSONAME_ACK] = 1;
    msg[JSONAME_TEMP] = info->targetTemp;
    return jsonobj2string(msg);
}

//前台开房
QString Console::rcvType6(const QJsonObject& json)
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

//前台退房
QString Console::rcvType7(const QJsonObject& json)
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

//获取所有空调状态
QString Console::rcvType10()
{
    //返回包
    QJsonObject msg;
    msg[JSONAME_TYPE] = 10;

    QJsonObject airStatus;
    for(auto iter : mapLinkInfo)
    {
        if(iter.roomID.isEmpty()) continue;

        QJsonObject info;
        info[JSONAME_TEMP] = iter.targetTemp;
        info[JSONAME_WNDSPD] = iter.targetWndSpd;

        auto infoString = jsonobj2string(info);
        airStatus[iter.roomID] = infoString;
    }

    msg[JSONAME_AIRSTATUS] = jsonobj2string(airStatus);
    return jsonobj2string(msg);
}

QMap<QString, LinkInfo>::iterator Console::getLinkInfo(QString& idt)
{
    return mapLinkInfo.find(idt);
}

void Console::process(const QString& msg)
{
    QJsonObject json = string2jsonobj(msg);
    QWebSocket* clt = qobject_cast<QWebSocket*>(sender());
    QString idt = getIdentifier(clt);
    auto info_iter = getLinkInfo(idt);

#ifdef DEBUG_RCV_CONTENT
    qDebug() << "rcv: " << msg;
#endif

    //按type域分类处理
    int opt = json[JSONAME_TYPE].toInt();
    QString jsonRet;
    switch(opt)
    {
    case 0:
        jsonRet = rcvType0(json, info_iter);
        break;
    case 1:
        jsonRet = rcvType1(json, info_iter);
        break;
    case 2:
        jsonRet = rcvType2(json, info_iter);
        break;
    case 3:
        jsonRet = rcvType3(json, info_iter);
        break;
    case 6:
        break;
    case 7:
        break;

    case 10:
        jsonRet = rcvType10();
        break;
    default:
        qDebug() << "Unknown OptType";
    }

#ifdef DEBUG_SEND_CONTENT
    qDebug() << "Send to " << idt << " : " << jsonRet;
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
