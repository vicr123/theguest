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
#include "guestmanager.h"

#include <QDir>
#include <QStandardPaths>
#include <QJsonObject>
#include <QJsonDocument>
#include "session/guestsession.h"

struct GuestManagerPrivate {
    QMap<QString, GuestSession*> sessions;
};

GuestManager* GuestManager::instance() {
    static GuestManager* manager = new GuestManager();
    return manager;
}

QString GuestManager::guestPath() {
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)).absoluteFilePath("guests");
}

QString GuestManager::guestPath(QString guest) {
    return QDir(this->guestPath()).absoluteFilePath(guest);
}

QStringList GuestManager::guestList() {
    return QDir(this->guestPath()).entryList(QDir::Dirs | QDir::NoDotAndDotDot);
}

QJsonObject GuestManager::guestSettings(QString guest) {
    QDir guestDir(this->guestPath(guest));
    if (!guestDir.exists()) return {};

    QFile configFile(guestDir.absoluteFilePath("machine.json"));
    if (!configFile.exists()) return {};

    configFile.open(QFile::ReadOnly);
    return QJsonDocument::fromJson(configFile.readAll()).object();
}

void GuestManager::setGuestSettings(QString guest, QJsonObject settings) {
    QDir guestDir(this->guestPath(guest));
    if (!guestDir.exists()) QDir::root().mkpath(guestDir.path());

    QFile configFile(guestDir.absoluteFilePath("machine.json"));
    configFile.open(QFile::WriteOnly);
    configFile.write(QJsonDocument(settings).toJson());
    configFile.close();

    emit guestsUpdated();
}

void GuestManager::deleteGuest(QString guest) {
    QDir guestDir(this->guestPath(guest));
    if (!guestDir.exists()) return;

    GuestSession* session = guestSession(guest);
    if (session->state() != GuestSession::Off) {
        //Stop the session before removing it
        session->forceStop();
    }

    guestDir.removeRecursively();

    emit guestsUpdated();
}

GuestSession* GuestManager::guestSession(QString guest) {
    if (d->sessions.contains(guest)) return d->sessions.value(guest);

    GuestSession* session = new GuestSession(guest);
    d->sessions.insert(guest, session);
    return session;
}

QString GuestManager::suggestBackendName(QString readableGuestName) {
    QString processed;
    for (QChar c : readableGuestName) {
        if (c == ' ') {
            processed += "-";
        } else if (c.unicode() < 256) {
            processed += c.toLower();
        }
    }

    if (!guestList().contains(processed)) return processed;

    QString processedWithNumber;
    int num = 1;
    do {
        processed = QStringLiteral("%1-%2").arg(processed).arg(num);
    } while (guestList().contains(processedWithNumber));

    return processedWithNumber;
}

GuestManager::GuestManager(QObject* parent) : QObject(parent) {
    d = new GuestManagerPrivate();
    QDir::root().mkpath(this->guestPath());
}
