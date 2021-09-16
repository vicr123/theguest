/****************************************
 *
 *   INSERT-PROJECT-NAME-HERE - INSERT-GENERIC-NAME-HERE
 *   Copyright (C) 2021 Victor Tran
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * *************************************/
#include "mainwindow.h"

#include <QDir>
#include <tsettings.h>
#include <tapplication.h>

int main(int argc, char* argv[]) {
    tApplication a(argc, argv);

    QString dir = SYSTEM_PREFIX_DIRECTORY;

    if (QDir(QStringLiteral("%1/share/theguest/").arg(SYSTEM_PREFIX_DIRECTORY)).exists()) {
        a.setShareDir(QStringLiteral("%1/share/theguest/").arg(SYSTEM_PREFIX_DIRECTORY));
    } else if (QDir(QDir::cleanPath(QApplication::applicationDirPath() + "/../share/theguest/")).exists()) {
        a.setShareDir(QDir::cleanPath(QApplication::applicationDirPath() + "/../share/theguest/"));
    }
    a.installTranslators();

    a.setApplicationVersion("3.1");
    a.setGenericName(QApplication::translate("main", "QEMU Frontend"));
    a.setAboutDialogSplashGraphic(a.aboutDialogSplashGraphicFromSvg(":/icons/aboutsplash.svg"));
    a.setApplicationLicense(tApplication::Gpl3OrLater);
    a.setCopyrightHolder("Victor Tran");
    a.setCopyrightYear("2021");
    a.setOrganizationName("theSuite");
//    a.setApplicationUrl(tApplication::HelpContents, QUrl("https://help.vicr123.com/docs/theguest/intro"));
    a.setApplicationUrl(tApplication::Sources, QUrl("http://github.com/vicr123/theGuest"));
    a.setApplicationUrl(tApplication::FileBug, QUrl("http://github.com/vicr123/theGuest/issues"));
    a.addCopyrightLine(QApplication::translate("main", "Parts of this application are possible due to the work of Lindsay Mathieson."));
#ifdef T_BLUEPRINT_BUILD
    a.setApplicationIcon(QIcon(":/icons/theguest-blueprint.svg"));
    a.setApplicationName("theGuest Blueprint");
    a.setDesktopFileName("com.vicr123.theguest_blueprint");
#else
    a.setApplicationIcon(QIcon::fromTheme("theguest", QIcon(":/icons/theguest.svg")));
    a.setApplicationName("theGuest");
    a.setDesktopFileName("com.vicr123.theguest");
#endif

    a.registerCrashTrap();

#if defined(Q_OS_WIN)
    a.setWinApplicationClassId("{98fd3bc5-b39c-4c97-b483-4c95b90a7c39}");
    tSettings::registerDefaults(a.applicationDirPath() + "/defaults.conf");
#elif defined(Q_OS_MAC)
    tSettings::registerDefaults(a.macOSBundlePath() + "/Contents/Resources/defaults.conf");
    a.setQuitOnLastWindowClosed(false);
#else
    tSettings::registerDefaults(a.applicationDirPath() + "/defaults.conf");
    tSettings::registerDefaults("/etc/theSuite/theBeat/defaults.conf");
#endif

    a.ensureSingleInstance({});

    MainWindow w;
    w.show();

    return a.exec();
}
