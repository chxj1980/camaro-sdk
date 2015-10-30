#include "processimage.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc,argv);
    ProcessImage process;
    process.resize(1280,640);
    process.show();

    return app.exec();
}
