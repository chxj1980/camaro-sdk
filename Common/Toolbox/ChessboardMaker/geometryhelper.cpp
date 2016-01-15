#include "geometryhelper.h"

#include <QGuiApplication>
#include <QScreen>

GeometryHelper::GeometryHelper(QObject *parent) : QObject(parent)
{

}

QPoint GeometryHelper::getOrigin(QString display)
{
    QPoint origin(0,0);
    foreach (QScreen *screen, QGuiApplication::screens())
    {
        if (screen->name()!=display)
            continue;
        origin.setX(screen->geometry().x());
        origin.setY(screen->geometry().y());
    }
    return origin;
}

