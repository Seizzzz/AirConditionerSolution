#include "mainwindow.h"
#include "ui_mainwindow.h"

struct AirData
{
    int temp;
    int wndspd;
    int mode;

    AirData()
    {
        temp = 26;
        wndspd = 0;
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

static syncInfo old_state;
static syncInfo state;

void MainWindow::syncServer(int type)
{
    QJsonObject json;
    QJsonObject jsonAirData;

    json[JSONAME_TYPE] = type;
    switch(type)
    {
    case 1:
        json[JSONAME_ROOMID] = state.roomID;
        json[JSONAME_USERID] = state.userID;

        jsonAirData[JSONAME_TEMP] = state.airdata.temp;
        jsonAirData[JSONAME_WNDSPD] = state.airdata.wndspd;
        jsonAirData[JSONAME_MODE] = state.airdata.mode;
        auto jsonAirDataString = QString(QJsonDocument(jsonAirData).toJson());
        json[JSONAME_AIRDATA] = jsonAirDataString;
    }

    auto jsonString = QString(QJsonDocument(json).toJson());
    sock->sendTextMessage(jsonString);
}

void MainWindow::onConnected()
{
    isConnected = true;
    timerReconnect->stop();
#ifdef DEBUG_CONNECTION
    qDebug() << "connected";
#endif
}

void MainWindow::onDisconnect()
{
    isConnected = false;
    timerReconnect->start(1);
#ifdef DEBUG_CONNECTION
    qDebug() << "disconnected";
#endif
}

void MainWindow::onMsgRcv(const QString& msg)
{
    //state = return;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(msg.toLocal8Bit().data());
    QJsonObject json = jsonDoc.object();

    //process
    switch(json[JSONAME_TYPE].toInt())
    {
    case 1:
        if(json[JSONAME_ACK].toBool())
        {
            //suc
            break; // do nothing
        }
        else
        {
            //failed
            state = old_state;
        }
    }

    //update ui
    if(state.airdata.mode) ui->LCDTemp->setStyleSheet("color: green");
    else ui->LCDTemp->setStyleSheet("color: red");

    ui->LCDTemp->display(state.airdata.temp);
    ui->LCDWndspd->display(state.airdata.wndspd);
}

void MainWindow::connectSrv(QString ip, int port)
{
    QString path = QString("ws://%1:%2").arg(ip).arg(port);
    QUrl url = QUrl(path);

    sock->open(url);
}

MainWindow::MainWindow(QString ip, int port, QString room, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    sock(new QWebSocket()),
    isConnected(false)
{
    ui->setupUi(this);

    //timerReconnect
    timerReconnect = new QTimer(this);
    connect(timerReconnect, &QTimer::timeout, [=](){
#ifdef DEBUG_CONNECTION
        qDebug() << "try to connecting...";
#endif
        if(!isConnected) connectSrv(ip, port);
        timerReconnect->start(3000);
    });

    //socket
    connect(sock, &QWebSocket::connected, this, &MainWindow::onConnected);
    connect(sock, &QWebSocket::disconnected, this, &MainWindow::onDisconnect);
    connect(sock, &QWebSocket::textMessageReceived, this, &MainWindow::onMsgRcv);
    connectSrv(ip, port);

    //power
    connect(ui->pushButtonPower, &QPushButton::clicked, [=](){
        old_state = state;
        state.airdata.mode = !state.airdata.mode;
        syncServer(1);
    });

    //adjust temp
    connect(ui->pushButtonTempUp, &QPushButton::clicked, [=](){
        old_state = state;
        ++state.airdata.temp;
        syncServer(1);

    });
    connect(ui->pushButtonTempDown, &QPushButton::clicked, [=](){
        old_state = state;
        --state.airdata.temp;
        syncServer(1);
    });

    //adjust wndspd
    connect(ui->pushButtonWndspdUp, &QPushButton::clicked, [=](){
        old_state = state;
        ++state.airdata.wndspd;
        syncServer(1);

    });
    connect(ui->pushButtonWndspdDown, &QPushButton::clicked, [=](){
        old_state = state;
        --state.airdata.wndspd;
        syncServer(1);
    });
}

MainWindow::~MainWindow()
{
    delete ui;
    sock->close();
    sock->deleteLater();
}

