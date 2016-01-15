#ifndef RCSERVER_H
#define RCSERVER_H

#include <QObject>
#include <QList>
#include <QByteArray>
#include <QtWebSockets/QtWebSockets>

class RCServer : public QObject
{
    Q_OBJECT
    //Q_PROPERTY(QString message NOTIFY messageReceived)
public:
    explicit RCServer(QObject *parent = Q_NULLPTR);
    ~RCServer();

    Q_INVOKABLE void start(quint16 port);
    Q_INVOKABLE void stop();

Q_SIGNALS:
    void closed();
    void messageReceived(QString message);

private Q_SLOTS:
    void onNewConnection();
    //void processTextMessage(QString message);
    //void processBinaryMessage(QByteArray message);
    void socketDisconnected();

private:
    QWebSocketServer *m_pWebSocketServer;
    QList<QWebSocket *> m_clients;
    bool m_debug = true;
};

#endif // RCSERVER_H
