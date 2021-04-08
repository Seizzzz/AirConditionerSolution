#include "mainwindow.h"
#include "ui_mainwindow.h"

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
    int roomID;
    int userID;
    AirData airdata;
};

static int PriceCost;
static syncInfo lastState;
static syncInfo currState;

QJsonObject MainWindow::string2jsonobj(const QString& str)
{
    return QJsonDocument::fromJson(str.toLocal8Bit().data()).object();
}

QString MainWindow::jsonobj2string(const QJsonObject& obj)
{
    return QString(QJsonDocument(obj).toJson());
}

void MainWindow::syncServer(int type)
{
    QJsonObject json;

    json[JSONAME_TYPE] = type;
    switch(type)
    {
    case 1: //sync control info
    {
        json[JSONAME_ROOMID] = currState.roomID;
        json[JSONAME_USERID] = currState.userID;

        QJsonObject jsonAirData;
        //jsonAirData[JSONAME_POWER]
        jsonAirData[JSONAME_TEMP] = currState.airdata.temp;
        jsonAirData[JSONAME_WNDSPD] = currState.airdata.wndspd;
        jsonAirData[JSONAME_MODE] = currState.airdata.mode;
        json[JSONAME_AIRDATA] = jsonobj2string(jsonAirData);

        //will have sent the editted
        isControlInfoEditted = false;
        timerSendControlInfo->stop();
        break;
    }

    case 2:
    {
        json[JSONAME_ROOMID] = svRoomID;
        break;
    }
    }

    auto jsonString = jsonobj2string(json);
    sock->sendTextMessage(jsonString);
}

void MainWindow::onConnected()
{
    isConnected = true;
    timerReconnect->stop();
    timerGetPrice->start(INTERVAL_GETPRICE);
#ifdef DEBUG_CONNECTION
    qDebug() << "connected";
#endif
}

void MainWindow::onDisconnect()
{
    isConnected = false;
    timerReconnect->start(INTERVAL_RECONNECT);
#ifdef DEBUG_CONNECTION
    qDebug() << "disconnected";
#endif
}

void MainWindow::onMsgRcv(const QString& msg)
{
    auto json = string2jsonobj(msg);

    //process
    switch(json[JSONAME_TYPE].toInt())
    {
    case 1:
    {
        if(json[JSONAME_ACK].toBool()); //suc do nothing
        else currState = lastState; //failed

        break;
    }
    case 2:
    {
        PriceCost = json[JSONAME_MONEY].toInt();
        break;
    }
    }

    //update ui
    updateUI();
}

void MainWindow::connectSrv(QString ip, int port)
{
    QString path = QString("ws://%1:%2").arg(ip).arg(port);
    QUrl url = QUrl(path);

    sock->open(url);
}

void MainWindow::updateUI()
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

MainWindow::MainWindow(QString ip, int port, QString room, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
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
        if(isControlInfoEditted) syncServer(1);
    });

    //timerGetPrice
    timerGetPrice = new QTimer(this);
    connect(timerGetPrice, &QTimer::timeout, [=](){
#ifdef DEBUG_TIMER
        qDebug() << "timerGetPrice triggered";
#endif
        syncServer(2);
        timerGetPrice->start(INTERVAL_GETPRICE);
    });

    //socket
    connect(sock, &QWebSocket::connected, this, &MainWindow::onConnected);
    connect(sock, &QWebSocket::disconnected, this, &MainWindow::onDisconnect);
    connect(sock, &QWebSocket::textMessageReceived, this, &MainWindow::onMsgRcv);
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

MainWindow::~MainWindow()
{
    delete ui;
    sock->close();
    sock->deleteLater();
}

