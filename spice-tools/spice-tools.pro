QT += widgets multimedia

TEMPLATE = lib
CONFIG += staticlib

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    qspice-audio.cpp \
    qspice-channel.cpp \
    qspice-cursor-channel.cpp \
    qspice-display-channel.cpp \
    qspice-helper.cpp \
    qspice-inputs-channel.cpp \
    qspice-main-channel.cpp \
    qspice-object.cpp \
    qspice-playback-channel.cpp \
    qspice-port-channel.cpp \
    qspice-record-channel.cpp \
    qspice-session.cpp \
    qspice-smartcard-channel.cpp \
    qspice-smartcard-manager.cpp \
    qspice-smartcard-widget.cpp \
    qspice-usbdevice-manager.cpp \
    qspice-usbdevice-widget.cpp \
    qspice-usbredir-channel.cpp \
    qspice-webdav-channel.cpp \
    qspice-widget.cpp

HEADERS += \
    qspice-audio.h \
    qspice-channel.h \
    qspice-cursor-channel.h \
    qspice-display-channel.h \
    qspice-helper.h \
    qspice-inputs-channel.h \
    qspice-main-channel.h \
    qspice-object.h \
    qspice-playback-channel.h \
    qspice-port-channel.h \
    qspice-record-channel.h \
    qspice-session.h \
    qspice-smartcard-channel.h \
    qspice-smartcard-manager.h \
    qspice-smartcard-widget.h \
    qspice-usbdevice-manager.h \
    qspice-usbdevice-widget.h \
    qspice-usbredir-channel.h \
    qspice-webdav-channel.h \
    qspice-widget.h

unix:!macx {
    CONFIG += link_pkgconfig
    PKGCONFIG += spice-client-glib-2.0
}
