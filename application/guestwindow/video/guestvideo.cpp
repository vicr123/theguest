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
#include "guestvideo.h"
#include "ui_guestvideo.h"

#include "guestmanager.h"
#include "session/guestsession.h"
#include <QProcess>
#include <tlogger.h>

struct GuestVideoPrivate {
    QString guest;
    GuestSession* session;
};

GuestVideo::GuestVideo(QString guest, QWidget* parent) :
    QWidget(parent),
    ui(new Ui::GuestVideo) {
    ui->setupUi(this);
    d = new GuestVideoPrivate();
    d->guest = guest;
    d->session = GuestManager::instance()->guestSession(guest);

    connect(d->session, &GuestSession::stateChanged, this, &GuestVideo::updateState);
    updateState();

    connect(ui->spiceWidget, &QSpiceWidget::errMsg, this, [ = ](QString error) {
        tWarn("GuestVideo") << error;
    });
    connect(ui->spiceWidget, &QSpiceWidget::displayResized, this, [ = ](QSize size) {
        ui->spiceWidget->setFixedSize(size);
    });

    ui->stackedWidget->setCurrentAnimation(tStackedWidget::Fade);
}

GuestVideo::~GuestVideo() {
    delete d;
    delete ui;
}

void GuestVideo::sendKeySequence(Qt::Key keys) {
    ui->spiceWidget->sendKeySequence(keys);
}

void GuestVideo::on_startButton_clicked() {
    d->session->start();
}

void GuestVideo::updateState() {
    switch (d->session->state()) {
        case GuestSession::Off:
            ui->stackedWidget->setCurrentWidget(ui->offPage);
            break;
        case GuestSession::Starting:
        case GuestSession::Running:
            ui->stackedWidget->setCurrentWidget(ui->videoPage);
            attemptConnect();
            break;

    }
}

void GuestVideo::attemptConnect() {
    if (!ui->spiceWidget->connectToSpiceSource(d->session->spiceSocket(), "")) {
        //Try again in a bit
        QTimer::singleShot(100, this, &GuestVideo::attemptConnect);
        return;
    }

    ui->spiceWidget->setFocus();
}


void GuestVideo::on_viewVideoButton_clicked() {
    QProcess::startDetached("remote-viewer", {d->session->spiceSocket()});
}



void GuestVideo::resizeEvent(QResizeEvent* event) {
    ui->spiceWidget->updateSize(this->width(), this->height());
}
