#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QWebEngineView>
#include <QtWebEngine/QtWebEngine>
#include <QDebug>

#include "ipcserver.hpp"
#include "commandhandler.hpp"
#include "instantwebview.hpp"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    app.setApplicationName("instant-webview");
    app.setApplicationVersion("1.0");

    QCommandLineParser cmdParser;
    cmdParser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsCompactedShortOptions);
    cmdParser.setApplicationDescription("Instant WebView is a web browser which has no UI controls.");
    cmdParser.addHelpOption();
    cmdParser.addVersionOption();
    cmdParser.addOption(QCommandLineOption(QStringList() << "t" << "transport", "IPC Transport Layer to use.", "tcp|unixsocket|websocket"));
    cmdParser.addOption(QCommandLineOption(QStringList() << "r" << "reverse", "Enable reverse mode. The ID is used to identify your session in the server.", "ID"));
    cmdParser.process(app);

    if (cmdParser.value("transport").isEmpty()) {
        qDebug().noquote() << "You must provide a IPC transport layer";
        return -1;
    }

    IpcServer *ipcServer = new IpcServer();
    ipcServer->setTransport(cmdParser.value("transport"));
    ipcServer->setReverse(cmdParser.isSet("reverse"));
    ipcServer->setReverseId(cmdParser.value("reverse"));
    ipcServer->initialize();

    InstantWebView::instance()->initialize();

    QObject::connect(ipcServer, &IpcServer::newCommand, InstantWebView::instance()->commandHandler(), &CommandHandler::processCommand);

    return app.exec();
}
