#include "room.h"
#include "ui_room.h"

struct AirData
{
    int power;
    int temp;
    int wndspd;
    int mode;

    AirData()
    {
        temp = 26;
        wndspd = 1;
        mode = 0;
    }
};

struct syncInfo
{
    int type;
    QString roomID;
    QString userID;
    AirData airdata;
};

static int PriceCost;
static syncInfo lastState;
static syncInfo currState;

inline QJsonObject Room::string2jsonobj(const QString& str)
{
    return QJsonDocument::fromJson(str.toLocal8Bit().data()).object();
}

inline QString Room::jsonobj2string(const QJsonObject& obj)
{
    return QString(QJsonDocument(obj).toJson());
}

void Room::sendMsg(int type)
{
    QJsonObject json;

    json[JSONAME_TYPE] = type;
    switch(type)
    {
    //这里对开机与状态信息的改变进行了合并
    case 0:
    case 2:
    {
        json[JSONAME_ROOMID] = svRoomID;
        json[JSONAME_USERID] = svUserID;

        json[JSONAME_POWER] = currState.airdata.power;
        QJsonObject jsonAirData;
        jsonAirData[JSONAME_TEMP] = currState.airdata.temp;
        jsonAirData[JSONAME_WNDSPD] = currState.airdata.wndspd;
        jsonAirData[JSONAME_MODE] = currState.airdata.mode;
        json[JSONAME_AIRDATA] = jsonobj2string(jsonAirData);

        //与服务端同步控制信息
        //关闭定时器
        isControlInfoEditted = false;
        timerSendControlInfo->stop();

        break;
    }

    case 1:
    {
        //发送RoomID以获取UserID
        json[JSONAME_ROOMID] = svRoomID;

        break;
    }
    }

    auto jsonString = jsonobj2string(json);
    sock->sendTextMessage(jsonString);
}

void Room::onConnected()
{
    isConnected = true;
    timerReconnect->stop();
    timerGetPrice->start(INTERVAL_GETPRICE);
#ifdef DEBUG_CONNECTION
    qDebug() << "connected";
#endif

    //连接成功时，获取userID
    sendMsg(1);
}

void Room::onDisconnect()
{
    isConnected = false;
    timerReconnect->start(INTERVAL_RECONNECT);
    timerGetPrice->stop();
#ifdef DEBUG_CONNECTION
    qDebug() << "disconnected";
#endif
}

void Room::rcvType1(const QJsonObject& json)
{
    //获取UserID
    svUserID = json[JSONAME_USERID].toString();
}

void Room::rcvType2(const QJsonObject& json)
{
    //这是一个旧功能，用于确认服务端是否认可状态改变
    //出于潜在的安全性欠缺的考虑，该版本没有舍弃该功能
    if(!json[JSONAME_ACK].isUndefined())
    {
        if(json[JSONAME_ACK].toBool()); //成功改变，无需额外操作
        else currState = lastState; //退回上一次成功的状态
    }

    //获取当前账单金额
    PriceCost = json[JSONAME_MONEY].toInt();
}

void Room::onMsgRcv(const QString& msg)
{
    auto json = string2jsonobj(msg);
#ifdef DEBUG_RCV_CONTENT
    qDebug() << "rcv: " << msg;
#endif

    //process
    switch(json[JSONAME_TYPE].toInt())
    {
    case 1:
    {
        rcvType1(json);
        break;
    }
    case 2:
    {
        rcvType2(json);
        break;
    }
    }

    //update ui
    updateUI();
}

void Room::connectSrv(QString ip, int port)
{
    QString path = QString("ws://%1:%2").arg(ip).arg(port);
    QUrl url = QUrl(path);

    sock->open(url);
}

void Room::updateUI()
{
    if(currState.airdata.power)
    {
        if(currState.airdata.mode) ui->LCDTemp->setStyleSheet("color: green");
        else ui->LCDTemp->setStyleSheet("color: red");
    }
    else ui->LCDTemp->setStyleSheet("color: black");

    ui->LCDTemp->display(currState.airdata.temp);
    ui->LCDWndspd->display(currState.airdata.wndspd);
    ui->labelCostValue->setText(QString::number(PriceCost));
}

Room::Room(QString ip, int port, QString room,  QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Room),
    sock(new QWebSocket()),
    isConnected(false),
    isControlInfoEditted(false),
    svRoomID(room)
{
    ui->setupUi(this);

    //timerReconnect
    timerReconnect = new QTimer(this);
    connect(timerReconnect, &QTimer::timeout, [=](){
#ifdef DEBUG_TIMER
        qDebug() << "timerReconnect triggered";
#endif
#ifdef DEBUG_CONNECTION
        qDebug() << "try to connecting...";
#endif
        if(!isConnected) connectSrv(ip, port);
    });

    //timerSendControlInfo
    timerSendControlInfo = new QTimer(this);
    connect(timerSendControlInfo, &QTimer::timeout, [=](){
#ifdef DEBUG_TIMER
        qDebug() << "timerSendControlInfo triggered";
#endif
        //由于该版本每5s同步一次控制信息
        //暂时不需要此功能
        //if(isControlInfoEditted) sendMsg(2);
        isControlInfoEditted = false;
        timerSendControlInfo->stop();
    });

    //timerGetPrice
    timerGetPrice = new QTimer(this);
    connect(timerGetPrice, &QTimer::timeout, [=](){
#ifdef DEBUG_TIMER
        qDebug() << "timerGetPrice triggered";
#endif
        sendMsg(2);
        timerGetPrice->start(INTERVAL_GETPRICE);
    });

    //socket
    connect(sock, &QWebSocket::connected, this, &Room::onConnected);
    connect(sock, &QWebSocket::disconnected, this, &Room::onDisconnect);
    connect(sock, &QWebSocket::textMessageReceived, this, &Room::onMsgRcv);
    connectSrv(ip, port);

    //adjust power
    connect(ui->pushButtonPower, &QPushButton::clicked, [=](){
        lastState = currState;
        currState.airdata.power = !currState.airdata.power;
        isControlInfoEditted = true;
        timerSendControlInfo->start(INTERVAL_IMMEDIATE);

        updateUI();
    });

    //adjust mode
    connect(ui->pushButtonMode, &QPushButton::clicked, [=](){
        lastState = currState;
        currState.airdata.mode = !currState.airdata.mode;
        isControlInfoEditted = true;
        timerSendControlInfo->start(INTERVAL_IMMEDIATE);

        updateUI();
    });

    //adjust temp
    connect(ui->pushButtonTempUp, &QPushButton::clicked, [=](){
        if(VALMAX_TEMP < currState.airdata.temp + 1 || currState.airdata.temp + 1 < VALMIN_TEMP);
        else
        {
            if(!isControlInfoEditted) lastState = currState;
            ++currState.airdata.temp;
            isControlInfoEditted = true;
            timerSendControlInfo->start(INTERVAL_SYNCCONTROLINFO);

            updateUI();
        }
    });
    connect(ui->pushButtonTempDown, &QPushButton::clicked, [=](){
        if(VALMAX_TEMP < currState.airdata.temp - 1 || currState.airdata.temp - 1 < VALMIN_TEMP);
        else
        {
            if(!isControlInfoEditted) lastState = currState;
            --currState.airdata.temp;
            isControlInfoEditted = true;
            timerSendControlInfo->start(INTERVAL_SYNCCONTROLINFO);

            updateUI();
        }
    });

    //adjust wndspd
    connect(ui->pushButtonWndspdUp, &QPushButton::clicked, [=](){
        if(VALMAX_WNDSPD < currState.airdata.wndspd + 1 || currState.airdata.wndspd + 1 < VALMIN_WNDSPD);
        else
        {
            if(!isControlInfoEditted) lastState = currState;
            ++currState.airdata.wndspd;
            isControlInfoEditted = true;
            timerSendControlInfo->start(INTERVAL_SYNCCONTROLINFO);

            updateUI();
        }
    });
    connect(ui->pushButtonWndspdDown, &QPushButton::clicked, [=](){
        if(VALMAX_WNDSPD < currState.airdata.wndspd - 1 || currState.airdata.wndspd - 1 < VALMIN_WNDSPD);
        else
        {
            if(!isControlInfoEditted) lastState = currState;
            --currState.airdata.wndspd;
            isControlInfoEditted = true;
            timerSendControlInfo->start(INTERVAL_SYNCCONTROLINFO);

            updateUI();
        }
    });
}

Room::~Room()
{
    delete ui;
    sock->close();
    sock->deleteLater();
}

