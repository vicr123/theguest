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
#ifndef GUESTMANAGER_H
#define GUESTMANAGER_H

#include <QObject>

class GuestSession;
struct GuestManagerPrivate;
class GuestManager : public QObject {
        Q_OBJECT
    public:
        static GuestManager* instance();

        QString guestPath();
        QString guestPath(QString guest);

        QStringList guestList();
        QJsonObject guestSettings(QString guest);
        void setGuestSettings(QString guest, QJsonObject settings);
        void deleteGuest(QString guest);

        GuestSession* guestSession(QString guest);

        QString suggestBackendName(QString readableGuestName);

    signals:
        void guestsUpdated();

    private:
        explicit GuestManager(QObject* parent = nullptr);
        GuestManagerPrivate* d;

};

#endif // GUESTMANAGER_H
