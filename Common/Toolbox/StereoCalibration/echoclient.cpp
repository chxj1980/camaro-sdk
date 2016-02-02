#include "echoclient.h"
#include <QtCore/QDebug>

QT_USE_NAMESPACE

RCClient::RCClient(QObject *parent, bool debug) :
	QObject(parent),
	m_debug(debug)
{
	//if (m_debug)
	//	qDebug() << "WebSocket server:" << url;
	connect(&m_webSocket, &QWebSocket::connected, this, &RCClient::connected);
	connect(&m_webSocket, &QWebSocket::disconnected, this, &RCClient::closed);
	
}

RCClient::~RCClient()
{
	Disconnect();
}

bool RCClient::IsConnected() const
{
	return m_webSocket.state() == QAbstractSocket::ConnectedState;
}

void RCClient::Connect(QString ip)
{
	m_webSocket.open(QUrl(ip));
}

void RCClient::Disconnect()
{
	m_webSocket.close();
}


void RCClient::Send(QString message)
{
	if (m_webSocket.state()==QAbstractSocket::ConnectedState)
		m_webSocket.sendTextMessage(message);
}

//void RCClient::onConnected()
//{
//	if (m_debug)
//		qDebug() << "WebSocket connected";
//	connect(&m_webSocket, &QWebSocket::textMessageReceived,
//		this, &RCClient::onTextMessageReceived);
//	m_webSocket.sendTextMessage(QStringLiteral("Hello, world!"));
//}

//void RCClient::onTextMessageReceived(QString message)
//{
//	//if (m_debug)
//	//	qDebug() << "Message received:" << message;
//	//m_webSocket.close();
//}
