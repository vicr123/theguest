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
#ifndef GUESTITEM_H
#define GUESTITEM_H

#include <QWidget>

namespace Ui {
    class GuestItem;
}

struct GuestItemPrivate;
class GuestItem : public QWidget {
        Q_OBJECT

    public:
        explicit GuestItem(QString guest, QWidget* parent = nullptr);
        ~GuestItem();

    private:
        Ui::GuestItem* ui;
        GuestItemPrivate* d;

        void updateGuest();

        // QWidget interface
    protected:
        void mousePressEvent(QMouseEvent* event);
        void mouseReleaseEvent(QMouseEvent* event);
        void contextMenuEvent(QContextMenuEvent* event);
        void enterEvent(QEvent* event);
        void paintEvent(QPaintEvent* event);
        void leaveEvent(QEvent* event);
};

#endif // GUESTITEM_H
