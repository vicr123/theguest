QT       += core gui thelib multimedia
SHARE_APP_NAME = theguest
TARGET = theGuest

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

unix:!macx {
    DESKTOP_FILE = com.vicr123.theguest.desktop
    DESKTOP_FILE_BLUEPRINT = com.vicr123.theguest_blueprint.desktop

    # Include the-libs build tools
    equals(THELIBS_BUILDTOOLS_PATH, "") {
        THELIBS_BUILDTOOLS_PATH = $$[QT_INSTALL_PREFIX]/share/the-libs/pri
    }
    include($$THELIBS_BUILDTOOLS_PATH/buildmaster.pri)

    CONFIG += link_pkgconfig
    PKGCONFIG += glib-2.0 gio-2.0 spice-client-glib-2.0

    QT += thelib
    TARGET = theguest

    target.path = $$THELIBS_INSTALL_BIN

    defaults.files = defaults.conf
    defaults.path = $$THELIBS_INSTALL_SETTINGS/theSuite/theGuest/

    blueprint {
        metainfo.files = com.vicr123.theguest_blueprint.metainfo.xml
        icon.files = icons/com.vicr123.theguest_blueprint.svg
    } else {
        metainfo.files = com.vicr123.theguest.metainfo.xml
        icon.files = icons/com.vicr123.theguest.svg
    }

    INSTALLS += target icon defaults metainfo
}

SOURCES += \
    deletemachinepopover.cpp \
    guestitem.cpp \
    guestmanager.cpp \
    guestwindow/guestwindow.cpp \
    guestwindow/guestwindowmanager.cpp \
    guestwindow/settings/drivemodel.cpp \
    guestwindow/settings/drivespage.cpp \
    guestwindow/settings/guestsettings.cpp \
    guestwindow/settings/newdrivepopover.cpp \
    guestwindow/settings/settingsbinder.cpp \
    guestwindow/settings/settingsbinderhelper.cpp \
    guestwindow/video/guestvideo.cpp \
    integermaps.cpp \
    main.cpp \
    mainwindow.cpp \
    session/guestsession.cpp

HEADERS += \
    deletemachinepopover.h \
    guestitem.h \
    guestmanager.h \
    guestwindow/guestwindow.h \
    guestwindow/guestwindowmanager.h \
    guestwindow/settings/drivemodel.h \
    guestwindow/settings/drivespage.h \
    guestwindow/settings/guestsettings.h \
    guestwindow/settings/newdrivepopover.h \
    guestwindow/settings/settingsbinder.h \
    guestwindow/settings/settingsbinderhelper.h \
    guestwindow/video/guestvideo.h \
    integermaps.h \
    mainwindow.h \
    session/guestsession.h

FORMS += \
    deletemachinepopover.ui \
    guestitem.ui \
    guestwindow/guestwindow.ui \
    guestwindow/settings/drivespage.ui \
    guestwindow/settings/guestsettings.ui \
    guestwindow/settings/newdrivepopover.ui \
    guestwindow/video/guestvideo.ui \
    mainwindow.ui

RESOURCES += \
    resources.qrc

DISTFILES += \
    default-machine-definition.json

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../spice-tools/release/ -lspice-tools
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../spice-tools/debug/ -lspice-tools
else:unix: LIBS += -L$$OUT_PWD/../spice-tools/ -lspice-tools

INCLUDEPATH += $$PWD/../spice-tools
DEPENDPATH += $$PWD/../spice-tools

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../spice-tools/release/libspice-tools.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../spice-tools/debug/libspice-tools.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../spice-tools/release/spice-tools.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../spice-tools/debug/spice-tools.lib
else:unix: PRE_TARGETDEPS += $$OUT_PWD/../spice-tools/libspice-tools.a
