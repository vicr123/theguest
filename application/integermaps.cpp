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
#include "integermaps.h"

#include <QMap>

QString IntegerMaps::qemuInterface(int interface) {
    QMap<int, QString> mapping = {
        {Interface::VirtIO, "virtio"},
        {Interface::IDE, "ide"},
        {Interface::SCSI, "scsi"}
    };
    return mapping.value(interface);
}

QString IntegerMaps::humanReadableInterface(Interface interface) {
    QMap<Interface, QString> mapping = {
        {Interface::VirtIO, tr("VirtIO")},
        {Interface::IDE, tr("IDE")},
        {Interface::SCSI, tr("SCSI")}
    };
    return mapping.value(interface);
}
