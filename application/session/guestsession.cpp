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
#include "guestsession.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QProcess>
#include <QDir>
#include <QApplication>
#include <QFileSystemWatcher>
#include <QThread>
#include "guestmanager.h"
#include "integermaps.h"

struct GuestSessionPrivate {
    QString guest;
    GuestSession::State state = GuestSession::Off;

    QProcess* qemuProcess = nullptr;
};

GuestSession::GuestSession(QString guest, QObject* parent) : QObject(parent) {
    d = new GuestSessionPrivate();
    d->guest = guest;
}

GuestSession::~GuestSession() {
    delete d;
}

GuestSession::State GuestSession::state() {
    return d->state;
}

void GuestSession::start() {
    if (d->state != Off) return;

    d->state = Starting;
    emit stateChanged(Starting);

    //Start the VM
    QDir guestPath(GuestManager::instance()->guestPath(d->guest));
    QJsonObject definition = GuestManager::instance()->guestSettings(d->guest);

    QString spiceSocket = guestPath.absoluteFilePath("spice-socket");
    if (QFile::exists(spiceSocket)) QFile::remove(spiceSocket);

    QStringList args = {
        "-cpu",
        "host,migratable=on",
        "-enable-kvm",
        "-spice",
        QStringLiteral("unix=on,addr=%1,disable-ticketing=on").arg(spiceSocket),
        "-m",
        QStringLiteral("%1M").arg(definition.value("memory").toInt()),
        "-vga",
        "qxl"
    };

    int cores = definition.value("cpu").toObject().value("cores").toInt();
    if (cores == 0) cores = QThread::idealThreadCount() / 2;
    args.append({
        "-smp",
        QStringLiteral("%1").arg(cores)
    });

    if (definition.value("efi").toBool()) {
        args.append("-drive");
        args.append(QStringLiteral("if=pflash,media=disk,file=%1,readonly=on").arg("/usr/share/edk2-ovmf/x64/OVMF.fd"));
    }

    for (QJsonValue disk : definition.value("drives").toArray()) {
        args.append("-drive");
        QJsonObject diskObject = disk.toObject();

        QStringList parts;
        if (diskObject.value("type") == "auto") {
            parts.append(QStringLiteral("file=%1").arg(guestPath.absoluteFilePath(diskObject.value("file").toString())));
        } else {
            parts.append(QStringLiteral("file=%1").arg(diskObject.value("file").toString()));
        }

        parts.append(QStringLiteral("if=%1").arg(IntegerMaps::qemuInterface(diskObject.value("iface").toInt())));

        if (diskObject.value("type") == "cdrom") {
            parts.append("media=cdrom");
        }

        args.append(parts.join(","));
    }

    d->qemuProcess = new QProcess(this);
    d->qemuProcess->setProcessChannelMode(QProcess::ForwardedChannels);
    d->qemuProcess->start("qemu-system-x86_64", args);
    connect(d->qemuProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [ = ] {
        d->state = Off;
        emit stateChanged(d->state);

        QFile::remove(spiceSocket);
        d->qemuProcess->deleteLater();
    });
    connect(QApplication::instance(), &QCoreApplication::aboutToQuit, d->qemuProcess, [ = ] {
        d->qemuProcess->kill();
    });

    QFileSystemWatcher* spiceSocketWatcher = new QFileSystemWatcher(d->qemuProcess);
    spiceSocketWatcher->addPath(guestPath.path());
    connect(spiceSocketWatcher, &QFileSystemWatcher::directoryChanged, this, [ = ](QString path) {
        //The VM is now running
        if (QFile::exists(spiceSocket)) {
            d->state = Running;
            emit stateChanged(d->state);
        }
    });
}

void GuestSession::forceStop() {
    d->qemuProcess->kill();
}

QString GuestSession::spiceSocket() {
    QDir guestPath(GuestManager::instance()->guestPath(d->guest));
//    return QStringLiteral("spice+unix://%1").arg(guestPath.absoluteFilePath("spice-socket"));
    return guestPath.absoluteFilePath("spice-socket");
}
