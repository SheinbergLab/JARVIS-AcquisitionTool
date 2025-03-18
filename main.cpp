/*******************************************************************************
 * File:        main.cpp
 * Created:     23. October 2020
 * Author:      Timo Hueser
 * Contact:     timo.hueser@gmail.com
 * Copyright:   2021 Timo Hueser
 * License:     LGPL v3.0
 ******************************************************************************/

#include "DarkStyle.hpp"
#include "camerainterface.hpp"
#include "flirconfigbackend.hpp"
#include "globals.hpp"
#include "mainwindow.hpp"
#include "triggerinterface.hpp"
#include "tcpipserver.hpp"

#include <QApplication>
#include <QImage>
#include <QSettings>
#include <QSplashScreen>
#include <QRegularExpression>
#include <QDebug>
#include <iostream>

#include "tcpipserver.hpp"

Q_DECLARE_METATYPE(QList<int>)
Q_DECLARE_METATYPE(statusType)


// called when a TCP/IP message is received
void handleMessage(TcpipServer *server,
		   MainWindow *m_window,
		   const QString &clientAddress,
		   const QByteArray &message,
		   const int clientId) {

  qDebug() << "Processing message from" << clientAddress << "ID:" << clientId;

  // input should be "command arg1 arg2 arg3";
  QString input = message; 
  input.remove(QRegularExpression("[\r\n]"));

  QStringList tokens = input.split(' ', Qt::SkipEmptyParts);

  if (tokens.isEmpty()) {
    return;
  }

  QByteArray response = "0\n";
  QString command = tokens[0]; // The command
  QStringList arguments = tokens.mid(1); // The arguments

  if (command == "setFilename") {
    if (arguments.count() > 0) {
      m_window->setFilename(arguments[0]);
    }
    response = "1\n";
  }

  else if (command == "setRunMode") {
    if (arguments.count() > 0) {
      bool ok; // To check if the conversion is successful
      int mode = arguments[0].toInt(&ok); // Convert to int
      if (ok) {
	m_window->setRunMode(static_cast<ControlBar::Mode>(mode));
      }
    }
  }
  
  server->sendResponse(clientId, response);    
}


int main(int argc, char **argv) {
    qSetMessagePattern(QStringLiteral("%{time}"
                                      //"%{appname}"
                                      ": ["
                                      "%{if-debug}D%{endif}"
                                      "%{if-info}I%{endif}"
                                      "%{if-warning}W%{endif}"
                                      "%{if-critical}C%{endif}"
                                      "%{if-fatal}F%{endif}"
                                      "] "
                                      "%{message}"));

    QCoreApplication::setOrganizationName("Test");
    QCoreApplication::setOrganizationDomain("test");
    QCoreApplication::setApplicationName("Camera Controller");
    // qRegisterMetaTypeStreamOperators<QList<QString>>("QList<QString>");
    // qRegisterMetaTypeStreamOperators<QVector<QPoint>>("QVector<QPoint>");
    qRegisterMetaType<statusType>();
    QApplication app(argc, argv);
    app.setStyle(new DarkStyle);

    QStringList themeSearchPaths = {
        "/home/trackingsetup/Documents/AnnotationToolbox/IconThemes",
        "IconThemes", "../IconThemes",
        "/usr/local/share/JARVIS-AcquisitionTool/icons"};
    QIcon::setThemeSearchPaths(themeSearchPaths);
    QIcon::setThemeName("DarkIconTheme");

    // QPixmap pixmap("/home/trackingsetup/Pictures/Screenshot from 2021-04-21
    // 15-58-12.png");
    // QSplashScreen splash(pixmap);
    // splash.show();
    app.processEvents();
    // delayl(100);
    FlirConfigBackend::getInstance();

    TcpipServer server;
    const int port = 1234;

    MainWindow m_window;

    // Connect the signal to a handler using lambda to include server
    QObject::connect(&server, &TcpipServer::messageReceived,
		     [&server, &m_window](const QString &clientAddress,
					 const QByteArray &message,
					 int clientId) {
		       handleMessage(&server, &m_window,
				     clientAddress, message, clientId);});
    server.start(port);

    m_window.show();
    // splash.finish(&m_window);
    return app.exec();
}
