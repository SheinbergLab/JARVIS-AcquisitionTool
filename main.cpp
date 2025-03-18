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

  // test by toggle main tab
  static int tab_id = 0;
  tab_id = !tab_id;
  m_window->setMode(tab_id);

  QByteArray response = "Processed: " + message.toUpper();
  
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
