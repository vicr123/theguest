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
#include "deletemachinepopover.h"
#include "ui_deletemachinepopover.h"

#include <QJsonObject>
#include "guestmanager.h"

struct DeleteMachinePopoverPrivate {
    QString guest;
};

DeleteMachinePopover::DeleteMachinePopover(QString guest, QWidget* parent) :
    QWidget(parent),
    ui(new Ui::DeleteMachinePopover) {
    ui->setupUi(this);
    d = new DeleteMachinePopoverPrivate();

    d->guest = guest;
    ui->titleLabel->setBackButtonShown(true);
    ui->titleLabel->setText(tr("Delete %1").arg(QLocale().quoteString(GuestManager::instance()->guestSettings(guest).value("name").toString())));
    ui->deleteConfirmWidget->setFixedWidth(SC_DPI(600));
    ui->doDeleteButton->setProperty("type", "destructive");
}

DeleteMachinePopover::~DeleteMachinePopover() {
    delete ui;
}

void DeleteMachinePopover::on_doDeleteButton_clicked() {
    GuestManager::instance()->deleteGuest(d->guest);
    emit done();
}


void DeleteMachinePopover::on_titleLabel_backButtonClicked() {
    emit done();
}

