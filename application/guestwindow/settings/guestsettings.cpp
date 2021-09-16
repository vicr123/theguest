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
#include "guestsettings.h"
#include "ui_guestsettings.h"

#include "guestmanager.h"
#include <QJsonObject>
#include "settingsbinder.h"
#include "session/guestsession.h"

struct GuestSettingsPrivate {
    QString guest;
    SettingsBinder* binder;
};

GuestSettings::GuestSettings(QString guest, QWidget* parent) :
    QWidget(parent),
    ui(new Ui::GuestSettings) {
    ui->setupUi(this);
    d = new GuestSettingsPrivate;
    d->guest = guest;

    ui->leftPane->setFixedWidth(SC_DPI(300));

    ui->drivesPage->setGuest(guest);

    d->binder = new SettingsBinder();
    d->binder->bind(ui->machineNameLabel, "text", "name");
    d->binder->bind(ui->systemMemoryBox, "value", "memory");
    d->binder->bind(ui->systemCoresBox, "value", "cpu.cores");
    d->binder->bind(ui->drivesPage, "drives", "drives");
    d->binder->bind(ui->systemEfiBox, "checked", "efi");

    d->binder->load(GuestManager::instance()->guestSettings(guest));
    connect(d->binder, &SettingsBinder::shouldSave, this, [ = ] {
        GuestManager::instance()->setGuestSettings(guest, d->binder->save());
    });

    connect(GuestManager::instance()->guestSession(d->guest), &GuestSession::stateChanged, this, [ = ] {
        ui->machineOnWarning->setVisible(GuestManager::instance()->guestSession(d->guest)->state() != GuestSession::Off);
    });
    ui->machineOnWarning->setVisible(GuestManager::instance()->guestSession(d->guest)->state() != GuestSession::Off);
}

GuestSettings::~GuestSettings() {
    delete ui;
}

void GuestSettings::on_forceStopButton_clicked() {
    GuestManager::instance()->guestSession(d->guest)->forceStop();
}

