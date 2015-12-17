#include "processimage.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    ProcessImage w;
    w.resize(1280,480);
    w.show();
    return a.exec();
}
