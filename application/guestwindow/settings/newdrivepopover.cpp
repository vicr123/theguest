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
#include "newdrivepopover.h"
#include "ui_newdrivepopover.h"

#include "guestmanager.h"
#include <QDir>
#include <QProcess>

struct NewDrivePopoverPrivate {
    QString guest;
};

NewDrivePopover::NewDrivePopover(QString guest, QWidget* parent) :
    QWidget(parent),
    ui(new Ui::NewDrivePopover) {
    ui->setupUi(this);
    d = new NewDrivePopoverPrivate();
    d->guest = guest;

    ui->optionsWidget->setFixedWidth(SC_DPI(600));
    ui->titleLabel->setBackButtonShown(true);

    ui->stackedWidget->setCurrentAnimation(tStackedWidget::Fade);
    ui->spinner->setFixedSize(SC_DPI_T(QSize(32, 32), QSize));
}

NewDrivePopover::~NewDrivePopover() {
    delete d;
    delete ui;
}

void NewDrivePopover::on_titleLabel_backButtonClicked() {
    emit rejected();
}


void NewDrivePopover::on_addDriveButton_clicked() {
    ui->stackedWidget->setCurrentWidget(ui->loadingPage);

    QDir guestPath(GuestManager::instance()->guestPath(d->guest));
    QString fileName = this->driveFileName();

    QProcess* qemuImgProcess = new QProcess(this);
    qemuImgProcess->start("qemu-img", {"create", "-f", "qcow2", guestPath.absoluteFilePath(fileName), QStringLiteral("%1G").arg(ui->sizeBox->value())});
    connect(qemuImgProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [ = ] {
        qDebug() << qemuImgProcess->readAllStandardError();

        emit accepted({
            {"type", "auto"},
            {"iface", 0},
            {"file", fileName}
        });
        qemuImgProcess->deleteLater();
    });
}

QString NewDrivePopover::driveFileName() {
    QDir guestPath(GuestManager::instance()->guestPath(d->guest));
    uint i = 1;
    QString filename;
    do {
        filename = QStringLiteral("system-disk-%1.qcow2").arg(i++);
    } while (QFile::exists(guestPath.absoluteFilePath(filename)));
    return filename;
}

