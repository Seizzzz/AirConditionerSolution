#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "room.h"
#include "reception.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    roomID = ui->lineEditRoomID->text();
    srvIP = ui->lineEditIP->text();
    srvPort = ui->lineEditPort->text().toInt();

    //line edit
    connect(ui->lineEditRoomID, &QLineEdit::textEdited, [=](){
        roomID = ui->lineEditRoomID->text();
    });
    connect(ui->lineEditIP, &QLineEdit::textEdited, [=](){
        srvIP = ui->lineEditIP->text();
    });
    connect(ui->lineEditPort, &QLineEdit::textEdited, [=](){
        srvPort = ui->lineEditPort->text().toInt();
    });

    //push button
    connect(ui->pushButtonRoom, &QPushButton::clicked, [=](){
        //this->hide();
        auto* w = new Room(srvIP, srvPort, roomID);
        w->show();
    });
    connect(ui->pushButtonReception, &QPushButton::clicked, [=](){
        //this->hide();
        auto* w = new Reception(srvIP, srvPort);
        w->show();
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}
