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
#include "guestwindow.h"
#include "ui_guestwindow.h"

#include "guestmanager.h"
#include "session/guestsession.h"
#include <QJsonObject>
#include <thelpmenu.h>
#include <tcsdtools.h>

#include "video/guestvideo.h"
#include "settings/guestsettings.h"

struct GuestWindowPrivate {
    tCsdTools csd;
    QString guest;

    GuestVideo* videoPage;
    GuestSettings* settingsPage;

    GuestSession* session;

    QMenu* sendKeysMenu;
};

GuestWindow::GuestWindow(QString guest, QWidget* parent) :
    QMainWindow(parent),
    ui(new Ui::GuestWindow) {
    ui->setupUi(this);
    d = new GuestWindowPrivate();
    d->guest = guest;

    d->csd.installMoveAction(ui->topWidget);
    d->csd.installResizeAction(this);

    if (tCsdGlobal::windowControlsEdge() == tCsdGlobal::Left) {
        ui->leftCsdLayout->addWidget(d->csd.csdBoxForWidget(this));
    } else {
        ui->rightCsdLayout->addWidget(d->csd.csdBoxForWidget(this));
    }

    d->sendKeysMenu = new QMenu(this);
    d->sendKeysMenu->setTitle(tr("Send Keys"));
    for (int seq : QList<int>({
    Qt::CTRL | Qt::ALT | Qt::Key_Delete,
    Qt::CTRL | Qt::ALT | Qt::Key_F1,
    Qt::CTRL | Qt::ALT | Qt::Key_F2,
    Qt::CTRL | Qt::ALT | Qt::Key_F3,
    Qt::CTRL | Qt::ALT | Qt::Key_F4,
    Qt::CTRL | Qt::ALT | Qt::Key_F5,
    Qt::CTRL | Qt::ALT | Qt::Key_F6,
    Qt::CTRL | Qt::ALT | Qt::Key_F7
})) {
        d->sendKeysMenu->addAction(QKeySequence(seq).toString(), this, [ = ] {
            d->videoPage->sendKeySequence(static_cast<Qt::Key>(seq));
        });
    }

    QMenu* menu = new QMenu(this);
    menu->addAction(ui->actionPower_On);
    menu->addAction(ui->actionForce_Stop);
    menu->addMenu(d->sendKeysMenu);
    menu->addSeparator();
    menu->addMenu(new tHelpMenu(this));
    menu->addAction(ui->actionClose);

    ui->menuButton->setIconSize(SC_DPI_T(QSize(24, 24), QSize));
    ui->menuButton->setMenu(menu);

    QJsonObject definition = GuestManager::instance()->guestSettings(guest);
    ui->displayButton->setText(definition.value("name").toString());

    ui->stackedWidget->addWidget(d->videoPage = new GuestVideo(guest));
    ui->stackedWidget->addWidget(d->settingsPage = new GuestSettings(guest));
    ui->stackedWidget->setCurrentAnimation(tStackedWidget::SlideHorizontal);

    d->session = GuestManager::instance()->guestSession(guest);
    connect(d->session, &GuestSession::stateChanged, this, &GuestWindow::updateState);
    this->updateState();

    connect(GuestManager::instance(), &GuestManager::guestsUpdated, this, [ = ] {
        if (!GuestManager::instance()->guestList().contains(guest)) this->deleteLater();
    });
}

GuestWindow::~GuestWindow() {
    delete ui;
    d->settingsPage->deleteLater();
    delete d;
}

void GuestWindow::on_settingsButton_toggled(bool checked) {
    if (checked) ui->stackedWidget->setCurrentWidget(d->settingsPage);
}


void GuestWindow::on_displayButton_toggled(bool checked) {
    if (checked) ui->stackedWidget->setCurrentWidget(d->videoPage);
}


void GuestWindow::on_actionPower_On_triggered() {
    d->session->start();
}


void GuestWindow::on_actionForce_Stop_triggered() {
    d->session->forceStop();
}

void GuestWindow::updateState() {
    ui->actionPower_On->setEnabled(d->session->state() == GuestSession::Off);
    ui->actionForce_Stop->setEnabled(d->session->state() == GuestSession::Running);
    d->sendKeysMenu->setEnabled(d->session->state() == GuestSession::Running);
}

