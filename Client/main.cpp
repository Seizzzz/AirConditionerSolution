//#include "mainwindow.h"
#include "room.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

//    MainWindow w;
//    w.show();

    Room w("127.0.0.1", 62100, "101");
    w.show();

    return a.exec();
}
