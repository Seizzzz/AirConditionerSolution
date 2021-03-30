#include "mainwindow.h"
#include "ui_mainwindow.h"

struct syncInfo
{
    int type;
    int roomID;
    int userID;

    int mode;
    int wndspd;
    int temp;

    syncInfo()
    {
        type = -1;
        roomID = -1;
        userID = -1;
        mode = -1;
        wndspd = -1;
        temp = -1;
    }
};

static syncInfo state;

void MainWindow::syncServer(int type)
{
    QJsonObject json;

    json[JSONAME_TYPE] = type;
    switch(type)
    {
    default:
        json[JSONAME_ROOMID] = state.roomID;
        json[JSONAME_USERID] = state.userID;
        json[JSONAME_WNDSPD] = state.wndspd;
        json[JSONAME_MODE] = state.mode;
        json[JSONAME_TEMP] = state.temp;
    }

    auto jsonString = QString(QJsonDocument(json).toJson());

    sock->sendTextMessage(jsonString);
}

void MainWindow::onConnected()
{
#ifdef DEBUG_CONNECTED
    qDebug() << "connected";
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
    default:
        state.roomID = json[JSONAME_ROOMID].toInt();
        state.userID = json[JSONAME_USERID].toInt();
        state.wndspd = json[JSONAME_WNDSPD].toInt();
        state.mode = json[JSONAME_MODE].toInt();
        state.temp = json[JSONAME_TEMP].toInt();
    }

    //update
    if(state.mode) ui->LCDTemp->setStyleSheet("color: green");
    else ui->LCDTemp->setStyleSheet("color: black");

    ui->LCDTemp->display(state.temp);
    ui->LCDWndspd->display(state.wndspd);
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    sock(new QWebSocket())
{
    ui->setupUi(this);

    //socket
    sock = new QWebSocket();
    connect(sock, &QWebSocket::connected, this, &MainWindow::onConnected);
    connect(sock, &QWebSocket::textMessageReceived, this, &MainWindow::onMsgRcv);

    QString path = QString("ws://%1:%2").arg(QString(SRV_ADDR), QString(SRV_PORT));
    QUrl url = QUrl(path);

    sock->open(url);

    //power
    connect(ui->pushButtonPower, &QPushButton::clicked, [=](){
        state.mode = !state.mode;
        syncServer(1);
    });

    //adjust temp
    connect(ui->pushButtonTempUp, &QPushButton::clicked, [=](){
        ++state.temp;
        syncServer(1);

    });
    connect(ui->pushButtonTempDown, &QPushButton::clicked, [=](){
        --state.temp;
        syncServer(1);
    });

    //adjust wndspd
    connect(ui->pushButtonWndspdUp, &QPushButton::clicked, [=](){
        ++state.wndspd;
        syncServer(1);

    });
    connect(ui->pushButtonWndspdDown, &QPushButton::clicked, [=](){
        --state.wndspd;
        syncServer(1);
    });
}

MainWindow::~MainWindow()
{
    delete ui;
    sock->close();
    sock->deleteLater();
}

