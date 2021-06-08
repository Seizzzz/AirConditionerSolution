#include "reception.h"
#include "ui_reception.h"

void Reception::onConnected()
{
    ui->plainTextInfo->appendPlainText("connected\n");
}

void Reception::onDisconnect()
{
    ui->plainTextInfo->appendPlainText("disconnected\n");
}

void Reception::onMsgRcv(const QString& msg)
{
    auto json = string2jsonobj(msg);

    int opt = json["MsgType"].toInt();
    switch (opt)
    {
    case 3:
    {
        ui->plainTextInfo->appendPlainText("成功开房！");
        QString roomID = json["RoomId"].toString();
        QString userID = json["UserId"].toString();
        ui->plainTextInfo->appendPlainText(QString("顾客%1的房间号为：%2\n").arg(userID).arg(roomID));
    }
    case 4:
    {
        ui->plainTextInfo->appendPlainText("成功退房！");
        QString roomID = json["RoomId"].toString();
        QString userID = json["UserId"].toString();
        ui->plainTextInfo->appendPlainText(QString("退房顾客%1的房间号为：%2\n").arg(userID).arg(roomID));
    }
    }
}

void Reception::connectSrv(QString ip, int port)
{
    QString path = QString("ws://%1:%2").arg(ip).arg(port);
    QUrl url = QUrl(path);

    sock->open(url);
}

Reception::Reception(QString ip, int port, QWidget* parent) :
    QWidget(parent),
    ui(new Ui::Reception),
    sock(new QWebSocket())
{
    //关闭窗口时析构
    setAttribute(Qt::WA_DeleteOnClose);

    ui->setupUi(this);

    //socket
    connect(sock, &QWebSocket::connected, this, &Reception::onConnected);
    connect(sock, &QWebSocket::disconnected, this, &Reception::onDisconnect);
    connect(sock, &QWebSocket::textMessageReceived, this, &Reception::onMsgRcv);
    connectSrv(ip, port);

    connect(ui->pushButtonCheckIn, &QPushButton::clicked, [=](){
        QJsonObject json;
        json["MsgType"] = 3;
        json["UserId"] = ui->lineEditCheckIn->text();
        jsonobj2string(json);

        auto jsonString = jsonobj2string(json);
        sock->sendTextMessage(jsonString);
    });

    connect(ui->pushButtonCheckOut, &QPushButton::clicked, [=](){
        QJsonObject json;
        json["MsgType"] = 4;
        //json["Roomid"] = ui->lineEditCheckIn->text();
        json["RoomId"] = ui->lineEditCheckOut->text();
        jsonobj2string(json);

        auto jsonString = jsonobj2string(json);
        sock->sendTextMessage(jsonString);
    });
}

Reception::~Reception()
{
    delete ui;
    sock->close();
    sock->deleteLater();
}
