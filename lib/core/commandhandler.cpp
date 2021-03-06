#include "commandhandler.hpp"

#include <QWebEngineView>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>
#include <QFile>
#include <QEventLoop>
#include <QBuffer>
#include <QProcess>
#include <QDesktopWidget>
#include <QApplication>

#include "eventmanager.hpp"
#include "electricwebview.hpp"
#include "inputeventfilter.hpp"

#include <command/commandclient.hpp>

CommandHandler::CommandHandler(QObject *parent)
    : QObject(parent)
{
}

void CommandHandler::processCommand(const Command &command) const
{
    QWebEngineView *webView = ElectricWebView::instance()->webView();

    if (command.name() == "load") {
        if (command.arguments().isEmpty())
            webView->load(QUrl("about:blank"));
        else
            webView->load(QUrl(command.arguments().first()));
    } else if (command.name() == "stop") {
        webView->stop();
    } else if (command.name() == "reload") {
        webView->reload();
    } else if (command.name() == "back") {
        webView->back();
    } else if (command.name() == "forward") {
        webView->forward();
    } else if (command.name() == "open") {
        QString mode = command.arguments().value(0);

        if (mode == "maximized") {
            webView->showMaximized();
        } else if (mode == "fullscreen") {
            webView->setGeometry(qApp->desktop()->screenGeometry());
            webView->showFullScreen();
        }
    } else if (command.name() == "close") {
        webView->close();
    } else if (command.name() == "current_url") {
        command.sendResponse(webView->url().toString().toLocal8Bit());
    } else if (command.name() == "set_html") {
        QString type = command.arguments().value(0);
        QString value = command.arguments().mid(1, -1).join(' ');

        if (type == "string") {
            webView->page()->setHtml(value.toLocal8Bit());
        } else if (type == "file") {
            QFile file(value);
            file.open(QFile::ReadOnly);

            webView->page()->setHtml(file.readAll());
        }
    } else if (command.name() == "get_html") {
        QString format = command.arguments().value(0);

        QEventLoop loop;

        if (format == "html") {
            webView->page()->toHtml([&command, &loop](const QString &html) {
                if (!command.client().isNull()) {
                    command.sendResponse(QUrl::toPercentEncoding(html));

                    if (command.isGetter())
                        command.client()->close();
                }
                loop.quit();
            });
        } else if (format == "text") {
            webView->page()->toPlainText([&command, &loop](const QString &text) {
                if (!command.client().isNull()) {
                    command.sendResponse(QUrl::toPercentEncoding(text));

                    if (command.isGetter())
                        command.client()->close();
                }
                loop.quit();
            });
        } else {
            return;
        }

        loop.exec();
    } else if (command.name() == "current_title") {
        command.sendResponse(webView->title().toLocal8Bit());
    } else if (command.name() == "screenshot") {
        processScreenshotCommand(command);
    } else if (command.name() == "subscribe") {
        QString eventName = command.arguments().value(0);
        QStringList events = QStringList()
                << "title_changed"
                << "url_changed"
                << "load_started"
                << "load_finished"
                << "user_activity"
                << "info_message_raised"
                << "warning_message_raised"
                << "error_message_raised"
                << "feature_permission_requested";

        if (events.contains(eventName)) {
            Event event(command);
            event.setName(eventName);

            ElectricWebView::instance()->eventManager()->subscribe(event);
        }
    } else if (command.name() == "exec_js") {
        processJavaScriptCommand(command);
    } else if (command.name() == "inject_js") {
        QMap<QString, QWebEngineScript::ScriptWorldId> worlds;
        worlds["main"] = QWebEngineScript::MainWorld;
        worlds["application"] = QWebEngineScript::ApplicationWorld;
        worlds["user"] = QWebEngineScript::UserWorld;

        QMap<QString, QWebEngineScript::InjectionPoint> injectionPoints;
        injectionPoints["document_creation"] = QWebEngineScript::DocumentCreation;
        injectionPoints["document_ready"] = QWebEngineScript::DocumentReady;
        injectionPoints["deferred"] = QWebEngineScript::Deferred;

        QWebEngineScript::ScriptWorldId world = worlds[command.arguments().value(0)];
        QWebEngineScript::InjectionPoint injectionPoint = injectionPoints[command.arguments().value(1)];

        QWebEngineScript script;
        script.setWorldId(world);
        script.setInjectionPoint(injectionPoint);

        QString source = command.arguments().value(2);
        QString value = command.arguments().mid(3, -1).join(' ');

        if (source == "string") {
            script.setSourceCode(value);
        } else if (source == "file") {
            QFile file(value);
            file.open(QFile::ReadOnly);

            script.setSourceCode(file.readAll());
        }

        ElectricWebView::instance()->webView()->page()->scripts().insert(script);
    } else if (command.name() == "idle_time") {
        command.sendResponse(QString("%1").arg(ElectricWebView::instance()->inputEventFilter()->idle()).toLocal8Bit());
    } else if (command.name() == "block_user_activity") {
        bool block = QVariant(command.arguments().value(0)).toBool();
        ElectricWebView::instance()->inputEventFilter()->setBlock(block);
    } else if (command.name() == "exec_cmd") {
        bool sync = command.arguments().value(0) == "sync";

        if (sync) {
            QProcess *process = new QProcess;
            process->start(command.arguments().mid(1, -1).join(' '));
            process->waitForFinished(-1);
            command.sendResponse(QUrl::toPercentEncoding(process->readAllStandardOutput()));
        } else {
            QProcess::startDetached(command.arguments().mid(1, -1).join(' '));
        }
    } else if (command.name() == "accept_feature_request" || command.name() == "reject_feature_request") {
        QMap<QString, QWebEnginePage::Feature> features;
        features["geolocation"] = QWebEnginePage::Geolocation;
        features["audio_capture"] = QWebEnginePage::MediaAudioCapture;
        features["video_capture"] = QWebEnginePage::MediaVideoCapture;
        features["audio_video_capture"] = QWebEnginePage::MediaAudioVideoCapture;
        features["mouse_lock"] = QWebEnginePage::MouseLock;

        QUrl securityOrigin(command.arguments().value(1));
        QWebEnginePage::Feature feature = features[command.arguments().value(0)];
        QWebEnginePage::PermissionPolicy policy;

        if (command.name() == "accept_feature_request")
            policy = QWebEnginePage::PermissionGrantedByUser;
        else
            policy = QWebEnginePage::PermissionDeniedByUser;

        ElectricWebView::instance()->webView()->page()->setFeaturePermission(securityOrigin, feature, policy);
    } else if (command.name() == "quit") {
        int exitCode = command.arguments().value(0).toInt();
        qApp->exit(exitCode);
    }
}

void CommandHandler::processScreenshotCommand(const Command &command) const
{
    QStringList region = command.arguments().value(0).split(',');

    int x = region.value(0).toInt();
    int y = region.value(1).toInt();
    int width = region.value(2).toInt();
    int height = region.value(3).toInt();

    QRect rect = QRect(QPoint(x, y), QSize(width, height));

    if (rect.isNull() || !rect.isValid())
        rect = QRect(QPoint(0, 0), QSize(-1, -1));

    QByteArray data;
    QBuffer buffer(&data);

    QPixmap pixmap = ElectricWebView::instance()->webView()->grab(rect);
    pixmap.save(&buffer, "JPG");

    command.sendResponse(data.toBase64());
}


void CommandHandler::processJavaScriptCommand(const Command &command) const
{
    QString type = command.arguments().value(0);
    QString value = command.arguments().mid(1, -1).join(' ');

    QEventLoop loop;

    // Process JavaScript response from Web View and tells the even loop to exit
    auto processJavaScriptResponse = [&command, &loop](const QVariant &out) mutable {
        if (!command.client().isNull()) {
            command.sendResponse(QUrl::toPercentEncoding(out.toString()));

            if (command.isGetter())
                command.client()->close();
        }
        loop.quit();
    };

    if (type == "string") {
        ElectricWebView::instance()->webView()->page()->runJavaScript(value, processJavaScriptResponse);
    } else if (type == "file") {
        QFile file(value);
        file.open(QFile::ReadOnly);

        ElectricWebView::instance()->webView()->page()->runJavaScript(file.readAll(), processJavaScriptResponse);
    } else {
        return;
    }

    // Wait for JavaScript response from the Web View
    loop.exec();
}
