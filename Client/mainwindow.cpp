#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "room.h"

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
        Room w(srvIP, srvPort, roomID);
        w.show();
    });

}

MainWindow::~MainWindow()
{
    delete ui;
}
