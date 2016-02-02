#ifndef ECHOCLIENT_H
#define ECHOCLIENT_H

#include <QObject>
#include <QtWebSockets/QWebSocket>

class RCClient : public QObject
{
	Q_OBJECT
public:
	explicit RCClient(QObject *parent = Q_NULLPTR, bool debug = false);
	~RCClient();
	void Connect(QString ip);
	void Disconnect();
	void Send(QString message);
	bool IsConnected() const;
Q_SIGNALS:
	void connected();
	void closed();

//private Q_SLOTS:
	//void onConnected();
	//void onTextMessageReceived(QString message);

private:
	QWebSocket m_webSocket;
	QUrl m_url;
	bool m_debug;
};

#endif // ECHOCLIENT_H
