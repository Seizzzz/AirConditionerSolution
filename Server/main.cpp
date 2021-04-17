#include "console.h"
#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    Console console(62000);

    return a.exec();
}
