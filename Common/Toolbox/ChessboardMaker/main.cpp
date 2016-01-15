#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QScreen>
#include <iostream>
#include <QQuickView>
#include <memory>

#include "rcserver.h"
#include "geometryhelper.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/chess-logo.png"));
    qmlRegisterType<RCServer>("com.dg.rcserver",1,0,"RCServer");
    qmlRegisterType<GeometryHelper>("com.dg.geometryhelper",1,0,"GeometryHelper");

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    return app.exec();
}
