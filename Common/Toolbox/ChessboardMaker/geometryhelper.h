#ifndef GEOMETRYHELPER_H
#define GEOMETRYHELPER_H

#include <QObject>
#include <QPoint>
#include <QString>

class GeometryHelper : public QObject
{
    Q_OBJECT
public:
    explicit GeometryHelper(QObject *parent = 0);
    Q_INVOKABLE QPoint getOrigin(QString display);
//signals:

//public slots:
};

#endif // GEOMETRYHELPER_H
