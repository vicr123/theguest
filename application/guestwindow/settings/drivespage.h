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
#ifndef DRIVESPAGE_H
#define DRIVESPAGE_H

#include <QWidget>
#include <QJsonValue>

namespace Ui {
    class DrivesPage;
}

struct DrivesPagePrivate;
class DrivesPage : public QWidget {
        Q_OBJECT
        Q_PROPERTY(QJsonValue drives READ drives WRITE setDrives NOTIFY drivesChanged)

    public:
        explicit DrivesPage(QWidget* parent = nullptr);
        ~DrivesPage();

        void setGuest(QString guest);

        void setDrives(QJsonValue drives);
        QJsonValue drives();

    signals:
        void drivesChanged(QJsonValue drives);

    private slots:
        void on_addDriveButton_clicked();

        void on_listView_customContextMenuRequested(const QPoint& pos);

        void on_attachDriveButton_clicked();

    private:
        Ui::DrivesPage* ui;
        DrivesPagePrivate* d;
};

#endif // DRIVESPAGE_H
