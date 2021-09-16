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
#include "guestwindowmanager.h"

#include <QMap>
#include "guestwindow.h"

struct GuestWindowManagerPrivate {
    QMap<QString, GuestWindow*> guestWindows;
};

GuestWindowManager* GuestWindowManager::instance() {
    static GuestWindowManager* manager = new GuestWindowManager();
    return manager;
}

GuestWindow* GuestWindowManager::guestWindow(QString guest) {
    if (d->guestWindows.contains(guest)) return d->guestWindows.value(guest);

    GuestWindow* win = new GuestWindow(guest);
    connect(win, &GuestWindow::destroyed, this, [ = ] {
        d->guestWindows.remove(guest);
    });
    d->guestWindows.insert(guest, win);
    win->show();

    return win;
}

GuestWindowManager::GuestWindowManager(QObject* parent) : QObject(parent) {
    d = new GuestWindowManagerPrivate;
}
