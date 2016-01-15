#include "rcserver.h"
#include <QtCore/QDebug>

//QT_USE_NAMESPACE

RCServer::RCServer(QObject *parent) :
    QObject(parent),
    m_pWebSocketServer(new QWebSocketServer(QStringLiteral("RC Server"),
                                            QWebSocketServer::NonSecureMode, this)),
    m_clients()
{
}

RCServer::~RCServer()
{
    stop();
}

void RCServer::start(quint16 port)
{
    if (m_pWebSocketServer->listen(QHostAddress::Any, port))
    {
        if (m_debug)
            qDebug() << "Echoserver listening on port" << port;
        connect(m_pWebSocketServer, &QWebSocketServer::newConnection,
                this, &RCServer::onNewConnection);
        connect(m_pWebSocketServer, &QWebSocketServer::closed,
                this, &RCServer::closed);
    }
}

void RCServer::stop()
{
    m_pWebSocketServer->close();
    qDeleteAll(m_clients.begin(), m_clients.end());
}

void RCServer::onNewConnection()
{
    QWebSocket *pSocket = m_pWebSocketServer->nextPendingConnection();

    connect(pSocket, &QWebSocket::textMessageReceived, this, &RCServer::messageReceived);
    //connect(pSocket, &QWebSocket::binaryMessageReceived, this, &RCServer::processBinaryMessage);
    connect(pSocket, &QWebSocket::disconnected, this, &RCServer::socketDisconnected);

    if (m_debug)
        qDebug() << "Socket Connected:" << pSocket->localAddress().toString();
    m_clients << pSocket;
}

//void RCServer::processTextMessage(QString message)
//{
//    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
//    if (m_debug)
//        qDebug() << "Message received:" << message;
//    if (pClient)
//    {
//        pClient->sendTextMessage(message);
//    }
//}

//void RCServer::processBinaryMessage(QByteArray message)
//{
//    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
//    if (m_debug)
//        qDebug() << "Binary Message received:" << message;
//    if (pClient) {
//        pClient->sendBinaryMessage(message);
//    }
//}

void RCServer::socketDisconnected()
{
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    if (m_debug)
        qDebug() << "socketDisconnected:" << pClient;
    if (pClient) {
        m_clients.removeAll(pClient);
        pClient->deleteLater();
    }
}
