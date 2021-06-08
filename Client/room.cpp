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

static double FeeRate;
static double PriceCost;
static syncInfo currState;

void Room::sendMsg(int type)
{
    QJsonObject json;

    json[JSONAME_TYPE] = type;
    switch(type)
    {
    case 0:
        break;

    case 1:
        //发送RoomID以获取UserID
        json[JSONAME_ROOMID] = svRoomID;
        break;

    case 2:
    {
        json[JSONAME_ROOMID] = svRoomID;
        json[JSONAME_USERID] = svUserID;
        json[JSONAME_POWER] = currState.airdata.power;
        QJsonObject jsonAirData;
        jsonAirData[JSONAME_TEMP] = currState.airdata.temp;
        jsonAirData[JSONAME_WNDSPD] = currState.airdata.wndspd;
        jsonAirData[JSONAME_MODE] = currState.airdata.mode;
        //json[JSONAME_AIRDATA] = jsonobj2string(jsonAirData);
        json.insert(JSONAME_AIRDATA, jsonAirData);

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
#ifdef DEBUG_CONNECTION
    qDebug() << "connected";
#endif

    ui->labelWelcomeErr->setText("已连接至服务端");
    ui->labelWelcomeErr->setStyleSheet("color: green");
}

void Room::onDisconnect()
{
    isConnected = false;
    timerReconnect->start(INTERVAL_RECONNECT);
#ifdef DEBUG_CONNECTION
    qDebug() << "disconnected";
#endif

    ui->stackedWidget->setCurrentIndex(0);

    ui->labelWelcomeErr->setText("无法连接至服务端");
    ui->labelWelcomeErr->setStyleSheet("color: red");
}

//开机返回包
void Room::rcvType0(const QJsonObject& json)
{
    currState.airdata.mode = json[JSONAME_MODE].toInt();
    currState.airdata.wndspd = json[JSONAME_WNDSPD].toInt();
    currState.airdata.temp = json[JSONAME_DEFAULTEMP].toInt();
    FeeRate = json[JSONAME_FEERATE].toDouble();
}

//获取UserID
void Room::rcvType1(const QJsonObject& json)
{
    svUserID = json[JSONAME_USERID].toString();
    if(svUserID != "-1")
        ui->stackedWidget->setCurrentIndex(1);
    else
    {
        ui->labelWelcomeErr->setText("未成功获取到房间信息，请联系前台");
        ui->labelWelcomeErr->setStyleSheet("color: red");
    }
}

//改变风速返回包
void Room::rcvType2(const QJsonObject& json)
{
    FeeRate = json[JSONAME_FEERATE].toDouble();
}

//改变温度返回包
void Room::rcvType3(const QJsonObject& json)
{
    currState.airdata.mode = json[JSONAME_TEMP].toInt();
}

//改变模式返回包
void Room::rcvType4(const QJsonObject& json)
{
    currState.airdata.mode = json[JSONAME_MODE].toInt();
}

//关机返回包
void Room::rcvType13(const QJsonObject& json)
{
    PriceCost = json[JSONAME_MONEY].toDouble();
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
    case 0:
        rcvType0(json);
        break;

    case 1:
        rcvType1(json);
        break;

    case 2:
        rcvType2(json);
        break;

    case 3:
        rcvType3(json);
        break;

    case 4:
        rcvType4(json);
        break;

    case 13:
        rcvType13(json);
        break;
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

void Room::reqEnter()
{
    sendMsg(1);
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
    ui->labelFeeValue->setText(QString::number(FeeRate));
}

Room::Room(QString ip, int port, QString room,  QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Room),
    sock(new QWebSocket()),
    isConnected(false),
    svRoomID(room)
{
    //关闭窗口时析构
    setAttribute(Qt::WA_DeleteOnClose);

    //init ui
    ui->setupUi(this);
    ui->labelRoomID->setText(room);
    ui->stackedWidget->setCurrentIndex(0);

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

    //socket
    connect(sock, &QWebSocket::connected, this, &Room::onConnected);
    connect(sock, &QWebSocket::disconnected, this, &Room::onDisconnect);
    connect(sock, &QWebSocket::textMessageReceived, this, &Room::onMsgRcv);
    connectSrv(ip, port);

    //welcome page
    connect(ui->pushButtonWelcome, &QPushButton::clicked, [=](){
        reqEnter();
    });
    connect(ui->stackedWidget, &QStackedWidget::currentChanged, [=](){
        qDebug() << "fuck";
    });

    //adjust power
    connect(ui->pushButtonPower, &QPushButton::clicked, [=](){
        currState.airdata.power = !currState.airdata.power;

        if(currState.airdata.power) sendMsg(0); //发送开机
        else sendMsg(13); //发送关机
        updateUI();
    });

    //adjust mode
    connect(ui->pushButtonMode, &QPushButton::clicked, [=](){
        currState.airdata.mode = !currState.airdata.mode;

        sendMsg(4);
        updateUI();
    });

    //adjust temp
    connect(ui->pushButtonTempUp, &QPushButton::clicked, [=](){
        ++currState.airdata.temp;

        sendMsg(3);
        updateUI();
    });
    connect(ui->pushButtonTempDown, &QPushButton::clicked, [=](){
        --currState.airdata.temp;

        sendMsg(3);
        updateUI();
    });

    //adjust wndspd
    connect(ui->pushButtonWndspdUp, &QPushButton::clicked, [=](){
        ++currState.airdata.wndspd;

        sendMsg(2);
        updateUI();
    });
    connect(ui->pushButtonWndspdDown, &QPushButton::clicked, [=](){
        --currState.airdata.wndspd;

        sendMsg(2);
        updateUI();
    });
}

Room::~Room()
{
    delete ui;
    sock->close();
    sock->deleteLater();
}

