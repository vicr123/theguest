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
#include "drivespage.h"
#include "ui_drivespage.h"

#include <tpopover.h>
#include "newdrivepopover.h"
#include "drivemodel.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QMenu>
#include <QFileDialog>
#include <QMessageBox>
#include "integermaps.h"

struct DrivesPagePrivate {
    QString guest;
    DriveModel* model;
};

DrivesPage::DrivesPage(QWidget* parent) :
    QWidget(parent),
    ui(new Ui::DrivesPage) {
    ui->setupUi(this);

    d = new DrivesPagePrivate();

}

DrivesPage::~DrivesPage() {
    delete ui;
}

void DrivesPage::setGuest(QString guest) {
    d->guest = guest;

    d->model = new DriveModel(guest);
    connect(d->model, &DriveModel::drivesUpdated, this, [ = ] {
        emit drivesChanged(d->model->drives());
    });
    ui->listView->setModel(d->model);
}

void DrivesPage::setDrives(QJsonValue drives) {
    d->model->setDrives(drives.toArray());
    emit drivesChanged(drives);
}

QJsonValue DrivesPage::drives() {
    return d->model->drives();
}

void DrivesPage::on_addDriveButton_clicked() {
    NewDrivePopover* newDrive = new NewDrivePopover(d->guest);
    tPopover* popover = new tPopover(newDrive);
    popover->setPopoverWidth(SC_DPI(-200));
    popover->setPopoverSide(tPopover::Bottom);
    connect(newDrive, &NewDrivePopover::rejected, popover, &tPopover::dismiss);
    connect(newDrive, &NewDrivePopover::accepted, this, [ = ](QJsonObject definition) {
        popover->dismiss();

        d->model->addDrive(definition);
    });
    connect(popover, &tPopover::dismissed, popover, &tPopover::deleteLater);
    connect(popover, &tPopover::dismissed, newDrive, &NewDrivePopover::deleteLater);
    popover->show(this->window());
}


void DrivesPage::on_listView_customContextMenuRequested(const QPoint& pos) {
    QModelIndexList selected = ui->listView->selectionModel()->selectedIndexes();

    QMenu* menu = new QMenu(this);
    if (selected.count() == 1) {
        menu->addSection(tr("For this disk"));
        menu->addAction(QIcon::fromTheme("list-remove"), tr("Remove"), [ = ] {
            d->model->removeDrive(ui->listView->selectionModel()->selectedIndexes().first());
        });
    }

    connect(menu, &QMenu::aboutToHide, menu, &QMenu::deleteLater);
    menu->popup(this->mapToGlobal(pos));
}


void DrivesPage::on_attachDriveButton_clicked() {
    QFileDialog* dialog = new QFileDialog(this);
    dialog->setAcceptMode(QFileDialog::AcceptOpen);
    dialog->setWindowTitle(tr("Attach Disk"));
    dialog->setLabelText(QFileDialog::Accept, tr("Attach"));
    connect(dialog, &QFileDialog::fileSelected, this, [ = ](QString file) {
        QString type;
        int iface;

        QFileInfo info(file);
        if (info.suffix() == "iso") {
            type = "cdrom";
            iface = IntegerMaps::Interface::IDE;
        } else if (info.suffix() == "qcow2") {
            type = "disk";
            iface = IntegerMaps::Interface::VirtIO;
        }

        if (type.isEmpty()) {
            QMessageBox::warning(this, tr("Unsupported Drive"), tr("This file is unsupported. Please try another file."));
        } else {
            d->model->addDrive({
                {"type", type},
                {"iface", iface},
                {"file", file}
            });
        }
    });
    connect(dialog, &QFileDialog::finished, dialog, &QFileDialog::deleteLater);
    dialog->open();
}

