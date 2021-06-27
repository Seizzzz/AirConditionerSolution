#include "reception.h"
#include "ui_reception.h"

void Reception::onConnected()
{
    qDebug() << "connected";
}

void Reception::onDisconnect()
{
    qDebug() << "disconnected";
}

void Reception::onMsgRcv(const QString& msg)
{
    qDebug() << "rcv: " << msg;
    auto json = string2jsonobj(msg);

    int opt = json[JSONAME_TYPE].toInt();
    switch (opt)
    {
    case 8: //账单
    {
        ui->tableWidgetBill->setRowCount(1);
        ui->tableWidgetBill->setItem(0, 0, new QTableWidgetItem(json[JSONAME_ROOMID].toString()));
        ui->tableWidgetBill->setItem(0, 1, new QTableWidgetItem(json[JSONAME_INTIME].toString()));
        ui->tableWidgetBill->setItem(0, 2, new QTableWidgetItem(json[JSONAME_OUTTIME].toString()));
        ui->tableWidgetBill->setItem(0, 3, new QTableWidgetItem(json[JSONAME_MONEY].toString()));
        break;
    }
    case 9: //详单
    {
        auto detail = string2jsonobj(json[JSONAME_DETAIL].toString());

        auto keys = detail.keys();
        int row = 0;
        for(auto record : keys)
        {
            auto info = string2jsonobj(detail[record].toString());
            ui->tableWidgetDetail->setRowCount(row+1);
            ui->tableWidgetDetail->setItem(row, 0, new QTableWidgetItem(info[JSONAME_REQSTARTIME].toString()));
            ui->tableWidgetDetail->setItem(row, 1, new QTableWidgetItem(info[JSONAME_REQENDTIME].toString()));
            ui->tableWidgetDetail->setItem(row, 2, new QTableWidgetItem(info[JSONAME_PARTFEE].toString()));
            ui->tableWidgetDetail->setItem(row, 2, new QTableWidgetItem(info[JSONAME_WNDSPD].toString()));
            ui->tableWidgetDetail->setItem(row, 2, new QTableWidgetItem(info[JSONAME_FEERATE].toString()));
            ++row;
        }
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

    connect(ui->pushButtonBill, &QPushButton::clicked, [=](){
        QJsonObject json;
        json[JSONAME_TYPE] = 8;
        json[JSONAME_ROOMID] = ui->lineEditRoomID->text();

        auto jsonString = jsonobj2string(json);
        sock->sendTextMessage(jsonString);
    });

    connect(ui->pushButtonDetail, &QPushButton::clicked, [=](){
        QJsonObject json;
        json[JSONAME_TYPE] = 9;
        json[JSONAME_ROOMID] = ui->lineEditRoomID->text();

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
