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
#ifndef GUESTWINDOW_H
#define GUESTWINDOW_H

#include <QMainWindow>

namespace Ui {
    class GuestWindow;
}

struct GuestWindowPrivate;
class GuestWindow : public QMainWindow {
        Q_OBJECT

    public:
        explicit GuestWindow(QString guest, QWidget* parent = nullptr);
        ~GuestWindow();

    private slots:
        void on_settingsButton_toggled(bool checked);

        void on_displayButton_toggled(bool checked);

        void on_actionPower_On_triggered();

        void on_actionForce_Stop_triggered();

    private:
        Ui::GuestWindow* ui;
        GuestWindowPrivate* d;

        void updateState();
};

#endif // GUESTWINDOW_H
