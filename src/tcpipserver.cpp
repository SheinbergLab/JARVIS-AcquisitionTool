#include "tcpipserver.hpp"

TcpipServer::TcpipServer(QObject *parent) : QObject(parent), m_server(nullptr)
{
  m_server = new QTcpServer(this);
  m_nextClientId = 0;
  connect(m_server, &QTcpServer::newConnection, 
	  this, &TcpipServer::newConnection);
}

TcpipServer::~TcpipServer()
{
  m_server->close();

  // Close all active client connections and close
  for (QTcpSocket* clientSocket : m_clients) {
    if (clientSocket->state() == QAbstractSocket::ConnectedState) {
      clientSocket->disconnectFromHost(); // Graceful disconnect
      if (clientSocket->state() != QAbstractSocket::UnconnectedState) {
	clientSocket->waitForDisconnected(3000); // Wait for disconnection (3 seconds max)
      }
    }
    clientSocket->deleteLater(); // Cleanup
  }
  m_clients.clear();
  
  delete m_server;
}

bool TcpipServer::start(int port)
{
  if (m_server->isListening()) {
    stop();
  }
  
  return m_server->listen(QHostAddress::Any, port);
}

void TcpipServer::stop()
{
  if (m_server->isListening()) {
    m_server->close();
    
    // Close all client connections
    for (QTcpSocket* socket : m_clients) {
      socket->close();
    }
  }
}

bool TcpipServer::isRunning() const
{
  return m_server->isListening();
}

void TcpipServer::newConnection()
{
  while (m_server->hasPendingConnections()) {
    QTcpSocket *clientSocket = m_server->nextPendingConnection();
    
    connect(clientSocket, &QTcpSocket::readyRead,
	    this, &TcpipServer::readyRead);
    connect(clientSocket, &QTcpSocket::disconnected,
	    this, &TcpipServer::clientDisconnected);
    
    int clientId = m_nextClientId++;	
    m_clients.insert(clientId, clientSocket); 
    
    //        qDebug() << "New client connected:" << clientSocket->peerAddress().toString();
  }
}

void TcpipServer::readyRead()
{
  QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());
  if (!clientSocket)
    return;
  
  QByteArray data = clientSocket->readAll();
  QString clientAddress = clientSocket->peerAddress().toString();
  
  // Find the client ID for this socket
  int clientId = -1;
  QMap<int, QTcpSocket*>::iterator it;
  for (it = m_clients.begin(); it != m_clients.end(); ++it) {
    if (it.value() == clientSocket) {
      clientId = it.key();
      break;
    }
  }
  
  if (clientId == -1) {
    qWarning() << "Could not identify client ID for socket";
    return;
  }
  
  qDebug() << "Received data from" << clientAddress 
	   << "(ID:" << clientId << "):" << data;
  
  // Emit signal with received message and client ID
  // Instead of responding immediately, we wait for sendResponse to be called
  emit messageReceived(clientAddress, data, clientId);
}

void TcpipServer::sendResponse(int clientId, const QByteArray &response)
{
  if (!m_clients.contains(clientId)) {
    qWarning() << "Cannot send response: Unknown client ID:" << clientId;
    return;
  }
  
  QTcpSocket *clientSocket = m_clients.value(clientId);
  if (clientSocket && clientSocket->state() == QAbstractSocket::ConnectedState) {
    qDebug() << "Sending response to client ID:" << clientId << ":" << response;
    clientSocket->write(response);
  } else {
    qWarning() << "Cannot send response: Client socket not connected. ID:" << clientId;
  }
}

void TcpipServer::clientDisconnected()
{
  QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());
  if (!clientSocket)
    return;
  
  // Find and remove the client from our map
  QMap<int, QTcpSocket*>::iterator it;
  for (it = m_clients.begin(); it != m_clients.end(); ++it) {
    if (it.value() == clientSocket) {
      int clientId = it.key();
      qDebug() << "Client disconnected:" << clientSocket->peerAddress().toString() 
	       << "ID:" << clientId;
      m_clients.remove(clientId);
      break;
    }
  }
  
  clientSocket->deleteLater();
}

