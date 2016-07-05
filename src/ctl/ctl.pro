QT = gui widgets webenginewidgets websockets network

CONFIG += c++11

TARGET = instant-webview-ctl

INCLUDEPATH += \
    $$PWD/../../lib

SOURCES += \
    main.cpp

LIBS += \
    -L$$OUT_PWD/../../lib/core -lcore \
    -L$$OUT_PWD/../../lib/ipc -lipc \
    -L$$OUT_PWD/../../lib/transport -ltransport

scripts.files = $$PWD/shellscript.sh
scripts.path = $$PREFIX/share/instant-webview/

target.path = $$PREFIX/bin

INSTALLS += target scripts