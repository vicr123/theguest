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
#include "drivemodel.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QDir>
#include <QFile>
#include "guestmanager.h"

struct DriveModelPrivate {
    QString guest;
    QJsonArray drives;
};

DriveModel::DriveModel(QString guest, QObject* parent)
    : QAbstractListModel(parent) {
    d = new DriveModelPrivate();
    d->guest = guest;
}

DriveModel::~DriveModel() {
    delete d;
}

void DriveModel::setDrives(QJsonArray drives) {
    d->drives = drives;
    emit dataChanged(index(0), index(rowCount()));
}

QJsonArray DriveModel::drives() {
    return d->drives;
}

void DriveModel::addDrive(QJsonObject drive) {
    d->drives.append(drive);
    emit dataChanged(index(0), index(rowCount()));
    emit drivesUpdated();
}

void DriveModel::removeDrive(QModelIndex index) {
    QJsonObject definition = d->drives.at(index.row()).toObject();
    if (definition.value("type") == "auto") {
        //Also delete the file
        QFile::remove(QDir(GuestManager::instance()->guestPath(d->guest)).absoluteFilePath(definition.value("file").toString()));
    }

    d->drives.removeAt(index.row());
    emit dataChanged(this->index(0), this->index(rowCount()));
    emit drivesUpdated();
}

int DriveModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;

    return d->drives.count();
}

QVariant DriveModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid()) return QVariant();

    QJsonObject definition = d->drives.at(index.row()).toObject();
    switch (role) {
        case Qt::DisplayRole:
            return definition.value("file");
    }

    return QVariant();
}
