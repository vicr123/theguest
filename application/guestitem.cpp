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
#include "guestitem.h"
#include "ui_guestitem.h"

#include <QMouseEvent>
#include <QJsonObject>
#include <QMenu>
#include <tpopover.h>
#include "guestmanager.h"
#include "session/guestsession.h"
#include "deletemachinepopover.h"

#include "guestwindow/guestwindowmanager.h"
#include "guestwindow/guestwindow.h"

struct GuestItemPrivate {
    QString guest;
    GuestSession* session;
};

GuestItem::GuestItem(QString guest, QWidget* parent) :
    QWidget(parent),
    ui(new Ui::GuestItem) {
    ui->setupUi(this);
    d = new GuestItemPrivate;
    d->guest = guest;
    d->session = GuestManager::instance()->guestSession(guest);

    connect(GuestManager::instance(), &GuestManager::guestsUpdated, this, &GuestItem::updateGuest);
    connect(d->session, &GuestSession::stateChanged, this, &GuestItem::updateGuest);
    this->updateGuest();
}

GuestItem::~GuestItem() {
    delete d;
    delete ui;
}

void GuestItem::updateGuest() {
    QJsonObject data = GuestManager::instance()->guestSettings(d->guest);
    ui->nameLabel->setText(data.value("name").toString());

    switch (d->session->state()) {
        case GuestSession::Off:
            ui->stateLabel->setText(tr("Off"));
            break;
        case GuestSession::Starting:
            ui->stateLabel->setText(tr("Starting"));
            break;
        case GuestSession::Running:
            ui->stateLabel->setText(tr("Running"));
            break;
    }
}


void GuestItem::mousePressEvent(QMouseEvent* event) {
    event->accept();
}

void GuestItem::mouseReleaseEvent(QMouseEvent* event) {
    event->accept();
    if (this->underMouse()) {
        QMainWindow* window = GuestWindowManager::instance()->guestWindow(d->guest);
        window->show();
        window->activateWindow();
    }
}

void GuestItem::contextMenuEvent(QContextMenuEvent* event) {
    QMenu* menu = new QMenu(this);
    menu->addSection(tr("For this Virtual Machine"));
    QAction* powerOnAction = menu->addAction(QIcon::fromTheme("media-playback-start"), tr("Power On"), [ = ] {
        d->session->start();
    });
    QAction* powerOffAction = menu->addAction(QIcon::fromTheme("application-exit"), tr("Force Stop"), [ = ] {
        d->session->forceStop();
    });
    menu->addSeparator();
    menu->addAction(QIcon::fromTheme("edit-delete"), tr("Delete"), [ = ] {
        DeleteMachinePopover* newDrive = new DeleteMachinePopover(d->guest);
        tPopover* popover = new tPopover(newDrive);
        popover->setPopoverWidth(SC_DPI(-200));
        popover->setPopoverSide(tPopover::Bottom);
        connect(newDrive, &DeleteMachinePopover::done, popover, &tPopover::dismiss);
        connect(popover, &tPopover::dismissed, popover, &tPopover::deleteLater);
        connect(popover, &tPopover::dismissed, newDrive, &DeleteMachinePopover::deleteLater);
        popover->show(this->window());
    });
    connect(menu, &QMenu::aboutToHide, menu, &QMenu::deleteLater);
    menu->popup(event->globalPos());

    powerOnAction->setEnabled(d->session->state() == GuestSession::Off);
    powerOffAction->setEnabled(d->session->state() != GuestSession::Off);
}


void GuestItem::enterEvent(QEvent* event) {
}

void GuestItem::paintEvent(QPaintEvent* event) {
}

void GuestItem::leaveEvent(QEvent* event) {
}
