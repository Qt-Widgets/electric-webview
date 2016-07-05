#ifndef INSTANTWEBVIEW_HPP
#define INSTANTWEBVIEW_HPP

#include <QString>

class QWebEngineView;

class EventManager;
class CommandHandler;
class InputEventFilter;

class InstantWebView
{
public:
    static InstantWebView *instance();

    void initialize();

    void runScript(const QString &transport, const QString &fileName);

    QWebEngineView *webView() const;
    void setWebView(QWebEngineView *webView);

    EventManager *eventManager() const;
    CommandHandler *commandHandler() const;
    InputEventFilter *inputEventFilter() const;

private:
    InstantWebView();

    QWebEngineView *m_webView;
    EventManager *m_eventManager;
    CommandHandler *m_commandHandler;
    InputEventFilter *m_inputEventFilter;
};

#endif // INSTANTWEBVIEW_HPP