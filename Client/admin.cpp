#include "admin.h"
#include "ui_admin.h"

void Admin::onConnected()
{
    qDebug() << "connected";
}

void Admin::onDisconnect()
{
    qDebug() << "disconnected";
}

void Admin::getInfo(const QJsonObject& json)
{
    auto airstatus = string2jsonobj(json[JSONAME_AIRSTATUS].toString());

    auto keys = airstatus.keys();
    int row = 0;
    for(auto room : keys)
    {
        auto info = string2jsonobj(airstatus[room].toString());

        ui->tableWidget->setRowCount(row+1);
        ui->tableWidget->setItem(row, 0, new QTableWidgetItem(room));
        ui->tableWidget->setItem(row, 1, new QTableWidgetItem(info[JSONAME_TEMP].toInt()));
        ui->tableWidget->setItem(row, 2, new QTableWidgetItem(info[JSONAME_WNDSPD].toInt()));

        ++row;
    }
}

void Admin::onMsgRcv(const QString& msg)
{
    qDebug() << "rcv: " << msg;
    auto json = string2jsonobj(msg);

    int opt = json["MsgType"].toInt();
    switch (opt)
    {
    case 10:
        getInfo(json);
        break;
    case 11:

        break;
    }
}

void Admin::connectSrv(QString ip, int port)
{
    QString path = QString("ws://%1:%2").arg(ip).arg(port);
    QUrl url = QUrl(path);

    sock->open(url);
}

Admin::Admin(QString ip, int port, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Admin),
    sock(new QWebSocket())
{
    //关闭窗口时析构
    setAttribute(Qt::WA_DeleteOnClose);

    ui->setupUi(this);

    //socket
    connect(sock, &QWebSocket::connected, this, &Admin::onConnected);
    connect(sock, &QWebSocket::disconnected, this, &Admin::onDisconnect);
    connect(sock, &QWebSocket::textMessageReceived, this, &Admin::onMsgRcv);
    connectSrv(ip, port);

    //获取空调状态
    connect(ui->pushButtonRefreshInfo, &QPushButton::clicked, [=](){
        QJsonObject json;
        json[JSONAME_TYPE] = 10;

        auto jsonString = jsonobj2string(json);
        sock->sendTextMessage(jsonString);
    });

    //更新配置信息
    connect(ui->comboBoxMode, &QComboBox::currentTextChanged, [=](){
        mode = ui->comboBoxMode->currentIndex();
    });
    connect(ui->lineEditDefTemp, &QLineEdit::textEdited, [=](){
        defaultTemp = ui->lineEditDefTemp->text().toInt();
    });
    connect(ui->lineEditDefWndSpd, &QLineEdit::textEdited, [=](){
        defaultWndSpd = ui->lineEditLowFee->text().toInt();
    });
    connect(ui->lineEditMaxTemp, &QLineEdit::textEdited, [=](){
        maxTemp = ui->lineEditMaxTemp->text().toInt();
    });
    connect(ui->lineEditMinTemp, &QLineEdit::textEdited, [=](){
        minTemp = ui->lineEditMinTemp->text().toInt();
    });
    connect(ui->lineEditLowFee, &QLineEdit::textEdited, [=](){
        lowFee = ui->lineEditLowFee->text().toDouble();
    });
    connect(ui->lineEditMidFee, &QLineEdit::textEdited, [=](){
        midFee = ui->lineEditMidFee->text().toDouble();
    });
    connect(ui->lineEditLowFee, &QLineEdit::textEdited, [=](){
        lowFee = ui->lineEditLowFee->text().toDouble();
    });

    connect(ui->pushButtonConfig, &QPushButton::clicked, [=](){
        QJsonObject json;
        json[JSONAME_TYPE] = 11;
        json[JSONAME_MODE] = QString::number(mode);
        json[JSONAME_DEFAULTEMP] = QString::number(defaultTemp);
        json[JSONAME_DEFAULTWNDSPD] = QString::number(defaultWndSpd);
        json[JSONAME_HIGHTEMP] = QString::number(maxTemp);
        json[JSONAME_LOWTEMP] = QString::number(minTemp);
        json[JSONAME_HIGHFEE] = QString::number(highFee);
        json[JSONAME_MIDFEE] = QString::number(midFee);
        json[JSONAME_LOWFEE] = QString::number(lowFee);

        auto jsonString = jsonobj2string(json);
        sock->sendTextMessage(jsonString);
    });
}

Admin::~Admin()
{
    delete ui;
    sock->close();
    sock->deleteLater();
}
