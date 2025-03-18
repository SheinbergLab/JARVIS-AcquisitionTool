// tcpipserver.hpp
#ifndef TCPIPSERVER_H
#define TCPIPSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QList>
#include <QMap>
#include <QDebug>

class TcpipServer : public QObject
{
    Q_OBJECT

public:
  explicit TcpipServer(QObject *parent = nullptr);
  bool start(int port = 1234);
  void stop();
  bool isRunning() const;
  
signals:
  // Signal emitted when a message is received
  void messageReceived(const QString &clientAddress,
		       const QByteArray &message, int clientId);
							       
public slots:
  // Slot to receive the response to send back
  void sendResponse(int clientId, const QByteArray &response);
							     
private slots:
  void newConnection();
  void readyRead();
  void clientDisconnected();
  
private:
  QTcpServer *m_server;
  QMap<int, QTcpSocket*> m_clients;
  int m_nextClientId;
};

#endif // TCPIPSERVER_H

